#include "wdpch.h"
#include "DX12RayTracingPipeline.h"
#include <dxcapi.h>

#include "DX12Helper.h"

#define MAX_RAYTRACING_PAYLOAD_SIZE 128
#define MAX_RAYTRACING_ATTRIBUTES_SIZE 8
#define MAX_RAYTRACING_RECURSION_DEPTH 5

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
    
    DX12RayTracingPipeline::DX12RayTracingPipeline(const String& name, ID3D12Device5* device, RootSignature* rootSignature, RayTracingShader* shader) : Pipeline(name)
    {
        CurrentPipelineType = PipelineType::RayTracing;
        rootSignature->CurrentPipelineType = PipelineType::Compute;

        CD3DX12_STATE_OBJECT_DESC raytracingPipeline(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
        
        LPCWSTR RayGenShaderName = L"RayGenShader";
        LPCWSTR ClosestHitShaderName = L"ClosestHitShader";
        LPCWSTR MissShaderName = L"MissShader";
        LPCWSTR HitGroupName = L"HitGroup";
        
        IDxcBlob* shaderData = (IDxcBlob*)shader->GetPlatformData();

        auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
        D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(shaderData->GetBufferPointer(), shaderData->GetBufferSize());
        lib->SetDXILLibrary(&libdxil);
        // Define which shader exports to surface from the library.
        // If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
        // In this sample, this could be omitted for convenience since the sample uses all shaders in the library. 
        {
            lib->DefineExport(RayGenShaderName);
            lib->DefineExport(ClosestHitShaderName);
            lib->DefineExport(MissShaderName);
        }
        
        // Triangle hit group
        // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
        // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
        auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        hitGroup->SetClosestHitShaderImport(ClosestHitShaderName);
        hitGroup->SetHitGroupExport(HitGroupName);
        hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
        
        // Shader config
        // Defines the maximum sizes in bytes for the ray payload and attribute structure.
        auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
        UINT payloadSize = MAX_RAYTRACING_PAYLOAD_SIZE;   // float4 color
        UINT attributeSize = MAX_RAYTRACING_ATTRIBUTES_SIZE; // float2 barycentrics
        shaderConfig->Config(payloadSize, attributeSize);
        
        // Global root signature
        // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
        auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        globalRootSignature->SetRootSignature((ID3D12RootSignature*)rootSignature->GetNativeObject());

        // Pipeline config
        // Defines the maximum TraceRay() recursion depth.
        auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
        // PERFOMANCE TIP: Set max recursion depth as low as needed 
        // as drivers may apply optimization strategies for low recursion depths. 
        UINT maxRecursionDepth = MAX_RAYTRACING_RECURSION_DEPTH; // ~ primary rays only. 
        pipelineConfig->Config(maxRecursionDepth);

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))) || options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            throw std::runtime_error("Raytracing is not supported on this device.");
        }

        HRESULT hr = device->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&NativePipeline));
        
        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        BuildShaderTables(device, RayGenShaderName, MissShaderName, HitGroupName);
    }

    DX12RayTracingPipeline::~DX12RayTracingPipeline()
    {
    }
}
