#include "wdpch.h"
#include "DX12RayTracingPipeline.h"
#include <dxcapi.h>

#include "DX12Helper.h"

#define MAX_RAYTRACING_PAYLOAD_SIZE 128
#define MAX_RAYTRACING_ATTRIBUTES_SIZE 8
#define MAX_RAYTRACING_RECURSION_DEPTH 8

namespace Waldem
{
    void DX12RayTracingPipeline::BuildShaderTables(ID3D12Device5* device, LPCWSTR RayGenShaderName, LPCWSTR MissShaderName, LPCWSTR HitGroupName)
    {
        // Shader identifiers
        void* rayGenShaderIdentifier;
        void* missShaderIdentifier;
        void* hitGroupShaderIdentifier;

        // Get shader identifiers from the state object
        UINT shaderIdentifierSize;
        {
            ID3D12StateObjectProperties* stateObjectProperties;
            HRESULT hr = NativePipeline->QueryInterface(IID_PPV_ARGS(&stateObjectProperties));
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to query ID3D12StateObjectProperties!");
            }

            rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(RayGenShaderName);
            missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(MissShaderName);
            hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(HitGroupName);
            shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        }

        // Helper function to create a shader table
        auto CreateShaderTable = [&](void* shaderIdentifier, UINT shaderRecordSize, void* rootArguments, UINT rootArgumentsSize) -> ID3D12Resource*
        {
            UINT totalSize = shaderRecordSize + rootArgumentsSize;
            
            // Create upload heap for the shader table
            D3D12_RESOURCE_DESC bufferDesc = {};
            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            bufferDesc.Width = totalSize;
            bufferDesc.Height = 1;
            bufferDesc.DepthOrArraySize = 1;
            bufferDesc.MipLevels = 1;
            bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
            bufferDesc.SampleDesc.Count = 1;
            bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

            ID3D12Resource* shaderTable;
            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&shaderTable)
            );
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to create shader table buffer!");
            }

            // Map and write shader identifier and root arguments
            void* mappedData = nullptr;
            shaderTable->Map(0, nullptr, &mappedData);
            memcpy(mappedData, shaderIdentifier, shaderIdentifierSize);
            if (rootArguments && rootArgumentsSize > 0)
            {
                memcpy(static_cast<BYTE*>(mappedData) + shaderIdentifierSize, rootArguments, rootArgumentsSize);
            }
            shaderTable->Unmap(0, nullptr);

            return shaderTable;
        };

        // Ray gen shader table
        {
            RayGenSBT = new ShaderBindingTable { CreateShaderTable(rayGenShaderIdentifier, shaderIdentifierSize, nullptr, 0), shaderIdentifierSize };
        }

        // Miss shader table
        {
            MissSBT = new ShaderBindingTable { CreateShaderTable(missShaderIdentifier, shaderIdentifierSize, nullptr, 0), shaderIdentifierSize };
        }

        // Hit group shader table
        {
            HitGroupSBT = new ShaderBindingTable { CreateShaderTable(hitGroupShaderIdentifier, shaderIdentifierSize, nullptr, 0), shaderIdentifierSize };
        }
    }
    
    DX12RayTracingPipeline::DX12RayTracingPipeline(const WString& name, ID3D12Device5* device, ID3D12RootSignature* rootSignature, RayTracingShader* shader) : Pipeline(name)
    {
        CurrentPipelineType = PipelineType::RayTracing;
        Device = device;
        RootSignature = rootSignature;
        ShaderObject = shader;
        if(!Reload())
        {
            throw std::runtime_error("Failed to create ray tracing pipeline state!");
        }
    }

    DX12RayTracingPipeline::~DX12RayTracingPipeline()
    {
    }

    void DX12RayTracingPipeline::Destroy()
    {
        if(NativePipeline != nullptr)
        {
            NativePipeline->Release();
            NativePipeline = nullptr;
        }
        if(RayGenSBT != nullptr)
        {
            if(RayGenSBT->Resource != nullptr) RayGenSBT->Resource->Release();
            delete RayGenSBT;
            RayGenSBT = nullptr;
        }
        if(MissSBT != nullptr)
        {
            if(MissSBT->Resource != nullptr) MissSBT->Resource->Release();
            delete MissSBT;
            MissSBT = nullptr;
        }
        if(HitGroupSBT != nullptr)
        {
            if(HitGroupSBT->Resource != nullptr) HitGroupSBT->Resource->Release();
            delete HitGroupSBT;
            HitGroupSBT = nullptr;
        }
    }

    bool DX12RayTracingPipeline::Reload()
    {
        CD3DX12_STATE_OBJECT_DESC raytracingPipeline(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

        LPCWSTR RayGenShaderName = L"RayGenShader";
        LPCWSTR ClosestHitShaderName = L"ClosestHitShader";
        LPCWSTR MissShaderName = L"MissShader";
        LPCWSTR HitGroupName = L"HitGroup";

        IDxcBlob* shaderData = (IDxcBlob*)ShaderObject->GetPlatformData();
        if(shaderData == nullptr)
        {
            return false;
        }

        auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(shaderData->GetBufferPointer(), shaderData->GetBufferSize());
        lib->SetDXILLibrary(&libdxil);
        lib->DefineExport(RayGenShaderName);
        lib->DefineExport(ClosestHitShaderName);
        lib->DefineExport(MissShaderName);

        auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        hitGroup->SetClosestHitShaderImport(ClosestHitShaderName);
        hitGroup->SetHitGroupExport(HitGroupName);
        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

        auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        shaderConfig->Config(MAX_RAYTRACING_PAYLOAD_SIZE, MAX_RAYTRACING_ATTRIBUTES_SIZE);

        auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        globalRootSignature->SetRootSignature(RootSignature);

        auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        pipelineConfig->Config(MAX_RAYTRACING_RECURSION_DEPTH);

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if(FAILED(Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))) || options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            return false;
        }

        ID3D12StateObject* newPipeline = nullptr;
        HRESULT hr = Device->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&newPipeline));
        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
            return false;
        }

        ShaderBindingTable* oldRayGen = RayGenSBT;
        ShaderBindingTable* oldMiss = MissSBT;
        ShaderBindingTable* oldHit = HitGroupSBT;
        ID3D12StateObject* oldPipeline = NativePipeline;

        NativePipeline = newPipeline;
        NativePipeline->SetName(DX12Helper::WFromMB(Name));
        BuildShaderTables(Device, RayGenShaderName, MissShaderName, HitGroupName);

        if(oldPipeline != nullptr)
        {
            oldPipeline->Release();
        }
        if(oldRayGen != nullptr)
        {
            if(oldRayGen->Resource != nullptr) oldRayGen->Resource->Release();
            delete oldRayGen;
        }
        if(oldMiss != nullptr)
        {
            if(oldMiss->Resource != nullptr) oldMiss->Resource->Release();
            delete oldMiss;
        }
        if(oldHit != nullptr)
        {
            if(oldHit->Resource != nullptr) oldHit->Resource->Release();
            delete oldHit;
        }

        return true;
    }
}
