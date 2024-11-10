#include "wdpch.h"
#include <d3dcompiler.h>
#include "DX12PixelShader.h"
#include "DX12Helper.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
#define MAX_TEXTURES 1024
#define MAX_BUFFERS 128
    
    DX12PixelShader::DX12PixelShader(const String& name, ID3D12Device* device, DX12CommandList* cmdList, std::vector<Resource> resources, Waldem::RenderTarget* renderTarget)
        : PixelShader(name, renderTarget), Device(device), CmdList(cmdList)
    {
        Device = device;
        CmdList = cmdList;

        if(CompileFromFile(name))
        {
            for (uint32_t i = 0; i < resources.size(); ++i)
            {
                D3D12_DESCRIPTOR_RANGE* range = new D3D12_DESCRIPTOR_RANGE();

                uint32_t numDescriptors = 1;

                if(resources[i].NumResources > 1)
                {
                    if(resources[i].Type == RTYPE_Texture)
                    {
                        numDescriptors = MAX_TEXTURES;
                    }
                    else if(resources[i].Type == RTYPE_Buffer)
                    {
                        numDescriptors = MAX_BUFFERS;
                    }
                }
                
                InitializedDescriptorsAmount += resources[i].NumResources;
                
                range->NumDescriptors = numDescriptors;
                range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                range->RegisterSpace = 0;
                range->RangeType = DX12Helper::ResourceTypeToRangeType(resources[i].Type);
                range->BaseShaderRegister = resources[i].Slot;
                
                D3D12_ROOT_PARAMETER param = {};
                param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                param.DescriptorTable.NumDescriptorRanges = 1;
                param.DescriptorTable.pDescriptorRanges = range;
                param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

                RootParams.push_back(param);
            }
            
            D3D12_STATIC_SAMPLER_DESC staticSampler = {};
            staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            staticSampler.MipLODBias = 0.0f;
            staticSampler.MaxAnisotropy = 1;
            staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSampler.MinLOD = 0.0f;
            staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
            staticSampler.ShaderRegister = 0; //matches register(s0) in the shader
            staticSampler.RegisterSpace = 0;
            staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            
            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.NumParameters = RootParams.size();
            rootSignatureDesc.pParameters = RootParams.data();
            rootSignatureDesc.NumStaticSamplers = 1;
            rootSignatureDesc.pStaticSamplers = &staticSampler;
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
            psoDesc.VS = { VertexShaderBlob->GetBufferPointer(), VertexShaderBlob->GetBufferSize() };
            psoDesc.PS = { PixelShaderBlob->GetBufferPointer(), PixelShaderBlob->GetBufferSize() };
            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
            psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            psoDesc.DepthStencilState.DepthEnable = TRUE;
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = renderTarget ? (DXGI_FORMAT)renderTarget->GetFormat() : DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "MESH_ID", 0, DXGI_FORMAT_R16_UINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
            hr = Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineState));

            if(FAILED(hr))
            {
                throw std::runtime_error("Failed to create pipeline state!");
            }

            SetResources(resources, InitializedDescriptorsAmount);
        }
    }

    DX12PixelShader::~DX12PixelShader()
    {
    }

    bool DX12PixelShader::CompileFromFile(const String& shaderName)
    {
        String entryPoint = "main"; //TODO: make this configurable?
        
        auto currentPath = GetCurrentFolder();
        
        std::wstring wCurrentPath = std::wstring(currentPath.begin(), currentPath.end());
        std::wstring wShaderName = std::wstring(shaderName.begin(), shaderName.end());
        
        size_t lastSlash = wShaderName.find_last_of(L"/\\");

        // Extract the directory part
        std::wstring pathToShaders = wCurrentPath + L"/Shaders/";

        // Extract the base name
        std::wstring baseName = wShaderName.substr(lastSlash + 1);
        
        std::wstring shaderPath = pathToShaders + baseName + L".vs.hlsl";
        String target = "vs_5_1";

        //vertex shader
        HRESULT hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &VertexShaderBlob, // Output shader bytecode
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
        target = "ps_5_1";
        
        hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &PixelShaderBlob, // Output shader bytecode
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

    void DX12PixelShader::SetResources(std::vector<Resource> resourceDescs, uint32_t numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = numDescriptors;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ResourcesHeap));
        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = ResourcesHeap->GetCPUDescriptorHandleForHeapStart();
        // D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        // samplerHeapDesc.NumDescriptors = 1;
        // samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        // samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        //
        // hr = Device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&SamplersHeap));
        // if(FAILED(hr))
        // {
        //     DX12Helper::PrintHResultError(hr);
        // }
        
        // D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = SamplersHeap->GetCPUDescriptorHandleForHeapStart();
        
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        bool HasSampler = false;
        
        for (auto& resourceDesc : resourceDescs)
        {
            for (uint32_t i = 0; i < resourceDesc.NumResources; ++i)
            {
                ID3D12Resource* resourceBuffer;

                uint32_t size;

                switch (resourceDesc.Type)
                {
                case RTYPE_Sampler:
                    {
                        HasSampler = true;
                        break;
                    }
                case RTYPE_ConstantBuffer:
                    {
                        D3D12_RESOURCE_DESC bufferDesc = {};
                        
                        size = (uint32_t)resourceDesc.Size.x;
                        
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
                case RTYPE_Buffer:
                    {
                        size = (uint32_t)resourceDesc.Size.x;
                            
                        if(resourceDesc.Buffers.size() != 0)
                        {
                            resourceBuffer = (ID3D12Resource*)resourceDesc.Buffers[i]->GetPlatformResource();
                        }
                        else
                        {
                            D3D12_RESOURCE_DESC bufferDesc = {};
                            
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
                        }
                        
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Buffer.FirstElement = 0;
                        srvDesc.Buffer.NumElements = (uint32_t)resourceDesc.Size.x / resourceDesc.Stride;
                        srvDesc.Buffer.StructureByteStride = resourceDesc.Stride;
                        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                        Device->CreateShaderResourceView(resourceBuffer, &srvDesc, handle);
                        
                        break;
                    }
                case RTYPE_Texture:
                    {
                        size = resourceDesc.Size.x * resourceDesc.Size.y;
                        
                        if(resourceDesc.Textures.size() != 0)
                        {
                            resourceBuffer = (ID3D12Resource*)resourceDesc.Textures[i]->GetPlatformResource();
                        }
                        else
                        {
                            D3D12_RESOURCE_DESC bufferDesc = {};
                            
                            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                            bufferDesc.Alignment = 0;
                            bufferDesc.Width = (uint32_t)resourceDesc.Size.x;
                            bufferDesc.Height = (uint32_t)resourceDesc.Size.y;
                            bufferDesc.DepthOrArraySize = 1;
                            bufferDesc.MipLevels = 1;
                            bufferDesc.SampleDesc.Count = 1;
                            bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                            bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                            bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

                            Device->CreateCommittedResource(
                                &heapProps,
                                D3D12_HEAP_FLAG_NONE,
                                &bufferDesc,
                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                nullptr,
                                IID_PPV_ARGS(&resourceBuffer));
                        }
                        
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Texture2D.MipLevels = 1;
                        srvDesc.Texture2D.MostDetailedMip = 0;
                        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                        Device->CreateShaderResourceView(resourceBuffer, &srvDesc, handle);
                        
                        break;
                    }
                case RTYPE_RenderTarget:
                    {
                        size = resourceDesc.Size.x * resourceDesc.Size.y;
                        
                        resourceBuffer = (ID3D12Resource*)resourceDesc.RT->GetPlatformResource();
                        
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        if(resourceDesc.RT->IsDepthStencilBuffer())
                        {
                            srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                        }
                        else
                        {
                            srvDesc.Format = (DXGI_FORMAT)resourceDesc.RT->GetFormat();
                        }
                            
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Texture2D.MipLevels = 1;
                        srvDesc.Texture2D.MostDetailedMip = 0;
                        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                        Device->CreateShaderResourceView(resourceBuffer, &srvDesc, handle);
                        
                        break;
                    }
                default:
                    break;
                }

                // if(!HasSampler)
                // {
                //     D3D12_SAMPLER_DESC samplerDesc = {};
                //     samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                //     samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                //     samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                //     samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                //     samplerDesc.MipLODBias = 0.0f;
                //     samplerDesc.MaxAnisotropy = 1;
                //     samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                //     samplerDesc.MinLOD = 0.0f;
                //     samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
                //
                //     Device->CreateSampler(&samplerDesc, samplerHandle);
                // }

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
                    memcpy(pMappedData, resourceDesc.Data, size);
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

    void DX12PixelShader::UpdateResourceData(String name, void* data)
    {
        if(data)
        {
            auto resourceData = Resources[name];

            if(resourceData)
            {
                //map and copy data
                UINT8* pMappedData;
                HRESULT hr = resourceData->DX12Resource->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
                if(FAILED(hr))
                {
                    DX12Helper::PrintHResultError(hr);
                }
                memcpy(pMappedData, data, resourceData->Desc.Size.x * resourceData->Desc.Size.y);
                resourceData->DX12Resource->Unmap(0, nullptr);
            }
        }
    }
}
