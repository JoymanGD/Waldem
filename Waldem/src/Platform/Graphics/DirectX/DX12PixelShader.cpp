#include "wdpch.h"
#include <d3dcompiler.h>
#include "DX12PixelShader.h"

#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    DX12PixelShader::DX12PixelShader(const std::string& shaderName, ID3D12Device* device, DX12CommandList* cmdList, std::vector<ResourceDesc> resources)
    {
        Device = device;
        CmdList = cmdList;
        
        if(CompileFromFile(shaderName))
        {
            std::vector<D3D12_DESCRIPTOR_RANGE> ranges;

            for (uint32_t i = 0; i < resources.size(); ++i)
            {
                D3D12_DESCRIPTOR_RANGE range = {};
                range.NumDescriptors = resources[i].NumResources;
                range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                range.RegisterSpace = 0;
                range.RangeType = DX12Helper::ResourceTypeToRangeType(resources[i].Type);
                range.BaseShaderRegister = resources[i].Slot;
                
                ranges.push_back(range);
            }

            D3D12_ROOT_PARAMETER rootParams[1];
            rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[0].DescriptorTable.NumDescriptorRanges = (uint32_t)ranges.size();
            rootParams[0].DescriptorTable.pDescriptorRanges = ranges.data();
            rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            
            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.NumParameters = _countof(rootParams);
            rootSignatureDesc.pParameters = rootParams;
            rootSignatureDesc.NumStaticSamplers = 0;
            rootSignatureDesc.pStaticSamplers = nullptr;
            rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            
            ID3DBlob* signature;
            ID3DBlob* error;
            D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
            HRESULT hr = Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));

            if(FAILED(hr))
            {
                throw std::runtime_error("Failed to create root signature!");
            }

            // Pipeline state
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = RootSignature;
            psoDesc.VS = { VertexShader->GetBufferPointer(), VertexShader->GetBufferSize() };
            psoDesc.PS = { PixelShader->GetBufferPointer(), PixelShader->GetBufferSize() };
            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
            hr = Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineState));

            if(FAILED(hr))
            {
                throw std::runtime_error("Failed to create pipeline state!");
            }

            SetResources(resources);
        }
    }

    DX12PixelShader::~DX12PixelShader()
    {
    }

    bool DX12PixelShader::CompileFromFile(const std::string& shaderName)
    {
        std::string entryPoint = "main"; //TODO: make this configurable?
        
        auto currentPath = GetCurrentFolder();
        
        std::wstring wCurrentPath = std::wstring(currentPath.begin(), currentPath.end());
        std::wstring wShaderName = std::wstring(shaderName.begin(), shaderName.end());
        
        size_t lastSlash = wShaderName.find_last_of(L"/\\");

        // Extract the directory part
        std::wstring pathToShaders = wCurrentPath + L"/Shaders/";

        // Extract the base name
        std::wstring baseName = wShaderName.substr(lastSlash + 1);
        
        std::wstring shaderPath = pathToShaders + baseName + L".vs.hlsl";
        std::string target = "vs_5_0";

        //vertex shader
        HRESULT hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &VertexShader, // Output shader bytecode
            &ErrorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (ErrorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)ErrorBlob->GetBufferPointer());
            }
            
            return false;
        }

        //pixel shader
        shaderPath = pathToShaders + baseName + L".ps.hlsl";
        target = "ps_5_0";
        
        hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &PixelShader, // Output shader bytecode
            &ErrorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (ErrorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)ErrorBlob->GetBufferPointer());
            }
            
            return false;
        }

        return true;
    }

    void DX12PixelShader::SetResources(std::vector<ResourceDesc> resourceDescs)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = (uint32_t)resourceDescs.size();
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ResourcesHeap));
        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = ResourcesHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
        
        for (auto& resourceDesc : resourceDescs)
        {
            for (uint32_t i = 0; i < resourceDesc.NumResources; ++i)
            {
                ID3D12Resource* resourceBuffer;
                uint32_t size = resourceDesc.Size;

                D3D12_RESOURCE_DESC bufferDesc = {};
                
                switch (resourceDesc.Type)
                {
                case ConstantBuffer:
                    {
                        size = (size + 255) & ~255; //align to 256 bytes
                        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                        bufferDesc.Alignment = 0;
                        bufferDesc.Width = size;
                        bufferDesc.Height = 1;
                        bufferDesc.DepthOrArraySize = 1;
                        bufferDesc.MipLevels = 1;
                        bufferDesc.SampleDesc.Count = 1;
                        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
                        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

                        Device->CreateCommittedResource(
                            &heapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&resourceBuffer));
                        
                        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
                        cbvDesc.BufferLocation = resourceBuffer->GetGPUVirtualAddress();
                        cbvDesc.SizeInBytes = size;
                        Device->CreateConstantBufferView(&cbvDesc, handle);
                        break;
                    }
                case Buffer:
                    {
                        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                        bufferDesc.Alignment = 0;
                        bufferDesc.Width = size;
                        bufferDesc.Height = 1;
                        bufferDesc.DepthOrArraySize = 1;
                        bufferDesc.MipLevels = 1;
                        bufferDesc.SampleDesc.Count = 1;
                        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
                        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

                        Device->CreateCommittedResource(
                            &heapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&resourceBuffer));
                        
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Buffer.FirstElement = 0;
                        srvDesc.Buffer.NumElements = resourceDesc.Size / resourceDesc.Stride;
                        srvDesc.Buffer.StructureByteStride = resourceDesc.Stride;
                        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                        Device->CreateShaderResourceView(resourceBuffer, &srvDesc, handle);
                        break;
                    }
                case Texture: // TODO: go on from here
                    {
                        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                        bufferDesc.Alignment = 0;
                        bufferDesc.Width = size;
                        bufferDesc.Height = 1;
                        bufferDesc.DepthOrArraySize = 1;
                        bufferDesc.MipLevels = 1;
                        bufferDesc.SampleDesc.Count = 1;
                        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
                        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

                        Device->CreateCommittedResource(
                            &heapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&resourceBuffer));
                        
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Buffer.FirstElement = 0;
                        srvDesc.Buffer.NumElements = resourceDesc.Size / resourceDesc.Stride;
                        srvDesc.Buffer.StructureByteStride = resourceDesc.Stride;
                        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                        Device->CreateShaderResourceView(resourceBuffer, &srvDesc, handle);
                        break;
                    }
                default: ;
                }

                Resources[resourceDesc.Name] = new ResourceData{ resourceBuffer, resourceDesc };

                if(resourceDesc.Data)
                {
                    //map and copy data
                    UINT8* pMappedData;
                    hr = resourceBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
                    if(FAILED(hr))
                    {
                        DX12Helper::PrintHResultError(hr);
                    }
                    memcpy(pMappedData, resourceDesc.Data, resourceDesc.Size);
                    resourceBuffer->Unmap(0, nullptr);
                }
                
                handle.ptr += descriptorSize;
            }
        }
    }

    void DX12PixelShader::SetSamplers(std::vector<SamplerData> samplers)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = (uint32_t)samplers.size();
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&SamplersHeap));
    }

    void DX12PixelShader::UpdateResourceData(std::string name, void* data)
    {
        if(data)
        {
            auto resourceData = Resources[name];

            if(resourceData)
            {
                //map and copy data
                UINT8* pMappedData;
                HRESULT hr = resourceData->Resource->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
                if(FAILED(hr))
                {
                    DX12Helper::PrintHResultError(hr);
                }
                memcpy(pMappedData, data, resourceData->Desc.Size);
                resourceData->Resource->Unmap(0, nullptr);
            }
        }
    }
}
