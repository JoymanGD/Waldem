#include "wdpch.h"
#include "DX12RootSignature.h"

#include "DX12Helper.h"

namespace Waldem
{
#define MAX_TEXTURES 1024
#define MAX_BUFFERS 128
    
    DX12RootSignature::DX12RootSignature(ID3D12Device* device, DX12GraphicCommandList* cmdList, WArray<Resource> resources) : CmdList(cmdList)
    {
        WArray<D3D12_ROOT_PARAMETER> rootParams;
            
            for (uint32_t i = 0; i < resources.Num(); ++i)
            {
                InitializedDescriptorsAmount += resources[i].NumResources;
                
                D3D12_ROOT_PARAMETER param = {};

                if(resources[i].Type == RTYPE_Constant)
                {
                    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                    param.Constants.ShaderRegister = resources[i].Slot;
                    param.Constants.RegisterSpace = 0;
                    param.Constants.Num32BitValues = resources[i].NumResources;
                    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                }
                else
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
                
                    range->NumDescriptors = numDescriptors;
                    range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                    range->RegisterSpace = 0;
                    range->RangeType = DX12Helper::ResourceTypeToRangeType(resources[i].Type);
                    range->BaseShaderRegister = resources[i].Slot;
                    
                    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    param.DescriptorTable.NumDescriptorRanges = 1;
                    param.DescriptorTable.pDescriptorRanges = range;
                    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                }

                rootParams.Add(param);
                RootParamTypes.Add(resources[i].Type);
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
            rootSignatureDesc.NumParameters = rootParams.Num();
            rootSignatureDesc.pParameters = rootParams.GetData();
            rootSignatureDesc.NumStaticSamplers = 1;
            rootSignatureDesc.pStaticSamplers = &staticSampler;
            rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        
        ID3DBlob* signature;
        ID3DBlob* error;
        D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        HRESULT hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&NativeRootSignature));

        if(FAILED(hr))
        {
            throw std::runtime_error("Failed to create root signature!");
        }

        SetResources(device, cmdList, resources, InitializedDescriptorsAmount);
    }

    DX12RootSignature::~DX12RootSignature()
    {
    }

    void DX12RootSignature::SetResources(ID3D12Device* device, DX12GraphicCommandList* cmdList, WArray<Resource> resourceDescs, uint32_t numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = numDescriptors;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&ResourcesHeap));
        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE handle = ResourcesHeap->GetCPUDescriptorHandleForHeapStart();
        
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = 1;
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        
        hr = device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&SamplersHeap));
        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }
        
        UINT samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = SamplersHeap->GetCPUDescriptorHandleForHeapStart();
        
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;
        
        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        defaultHeapProps.CreationNodeMask = 1;
        defaultHeapProps.VisibleNodeMask = 1;
        
        D3D12_RESOURCE_DESC dummyBufferDesc = {};
        dummyBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        dummyBufferDesc.Alignment = 0;
        dummyBufferDesc.Width = 1;
        dummyBufferDesc.Height = 1;
        dummyBufferDesc.DepthOrArraySize = 1;
        dummyBufferDesc.MipLevels = 1;
        dummyBufferDesc.SampleDesc.Count = 1;
        dummyBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        dummyBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        dummyBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        for (auto& resourceDesc : resourceDescs)
        {
            for (uint32_t i = 0; i < resourceDesc.NumResources; ++i)
            {
                ID3D12Resource* uploadResourceBuffer;
                ID3D12Resource* defaultResourceBuffer;

                uint32_t size;

                switch (resourceDesc.Type)
                {
                case RTYPE_Sampler:
                    {
                        auto& sampler = resourceDesc.Samplers[i];
                        
                        D3D12_SAMPLER_DESC samplerDesc = {};
                        samplerDesc.Filter = (D3D12_FILTER)sampler.Filter;
                        samplerDesc.AddressU = (D3D12_TEXTURE_ADDRESS_MODE)sampler.AddressU;
                        samplerDesc.AddressV = (D3D12_TEXTURE_ADDRESS_MODE)sampler.AddressV;
                        samplerDesc.AddressW = (D3D12_TEXTURE_ADDRESS_MODE)sampler.AddressW;
                        samplerDesc.MipLODBias = sampler.MipLODBias;
                        samplerDesc.MaxAnisotropy = sampler.MaxAnisotropy;
                        samplerDesc.ComparisonFunc = (D3D12_COMPARISON_FUNC)sampler.ComparisonFunc;
                        samplerDesc.MinLOD = sampler.MinLOD;
                        samplerDesc.MaxLOD = sampler.MaxLOD;
                        
                        device->CreateSampler(&samplerDesc, samplerHandle);
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

                        device->CreateCommittedResource(
                            &uploadHeapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&uploadResourceBuffer));

                        device->CreateCommittedResource(
                            &defaultHeapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                            nullptr,
                            IID_PPV_ARGS(&defaultResourceBuffer));
                        
                        cmdList->ResourceBarrier(defaultResourceBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                        
                        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
                        cbvDesc.BufferLocation = defaultResourceBuffer->GetGPUVirtualAddress();
                        cbvDesc.SizeInBytes = size;
                        device->CreateConstantBufferView(&cbvDesc, handle);
                        break;
                    }
                case RTYPE_Buffer:
                    {
                        size = (uint32_t)resourceDesc.Size.x;
                        
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

                        device->CreateCommittedResource(
                            &uploadHeapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&uploadResourceBuffer));
                            
                        if(resourceDesc.Buffers.Num() != 0)
                        {
                            // uploadResourceBuffer = (ID3D12Resource*)resourceDesc.Buffers[i]->GetPlatformResource();
                            defaultResourceBuffer = (ID3D12Resource*)resourceDesc.Buffers[i]->GetPlatformResource();
                        }
                        else
                        {
                            device->CreateCommittedResource(
                                &defaultHeapProps,
                                D3D12_HEAP_FLAG_NONE,
                                &bufferDesc,
                                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                nullptr,
                                IID_PPV_ARGS(&defaultResourceBuffer));
                        
                            cmdList->ResourceBarrier(defaultResourceBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                        }
                        
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Buffer.FirstElement = 0;
                        srvDesc.Buffer.NumElements = (uint32_t)resourceDesc.Size.x / resourceDesc.Stride;
                        srvDesc.Buffer.StructureByteStride = resourceDesc.Stride;
                        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
                        device->CreateShaderResourceView(defaultResourceBuffer, &srvDesc, handle);
                        
                        break;
                    }
                case RTYPE_Texture:
                    {
                        size = resourceDesc.Size.x * resourceDesc.Size.y;

                        device->CreateCommittedResource(
                            &uploadHeapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &dummyBufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&uploadResourceBuffer));
                        
                        if(resourceDesc.Textures.Num() != 0)
                        {
                            // uploadResourceBuffer = (ID3D12Resource*)resourceDesc.Textures[i]->GetPlatformResource();
                            defaultResourceBuffer = (ID3D12Resource*)resourceDesc.Textures[i]->GetPlatformResource();
                        }
                        else
                        {
                            D3D12_RESOURCE_DESC bufferDesc = {};
                        
                            device->CreateCommittedResource(
                                &defaultHeapProps,
                                D3D12_HEAP_FLAG_NONE,
                                &bufferDesc,
                                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                nullptr,
                                IID_PPV_ARGS(&defaultResourceBuffer));
                        
                            cmdList->ResourceBarrier(defaultResourceBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                        }
                        
                        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                        srvDesc.Texture2D.MipLevels = 1;
                        srvDesc.Texture2D.MostDetailedMip = 0;
                        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                        device->CreateShaderResourceView(defaultResourceBuffer, &srvDesc, handle);
                        
                        break;
                    }
                case RTYPE_RenderTarget:
                    {
                        size = resourceDesc.Size.x * resourceDesc.Size.y;

                        device->CreateCommittedResource(
                            &uploadHeapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &dummyBufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&uploadResourceBuffer));
                        
                        // uploadResourceBuffer = (ID3D12Resource*)resourceDesc.RT->GetPlatformResource();
                        defaultResourceBuffer = (ID3D12Resource*)resourceDesc.RT->GetPlatformResource();
                        
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
                        device->CreateShaderResourceView(defaultResourceBuffer, &srvDesc, handle);
                        
                        break;
                    }
                case RTYPE_RWRenderTarget:
                    {
                        size = resourceDesc.Size.x * resourceDesc.Size.y;

                        device->CreateCommittedResource(
                            &uploadHeapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &dummyBufferDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(&uploadResourceBuffer));
                        
                        // uploadResourceBuffer = (ID3D12Resource*)resourceDesc.RT->GetPlatformResource();
                        defaultResourceBuffer = (ID3D12Resource*)resourceDesc.RT->GetPlatformResource();
                        
                        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                        
                        if(resourceDesc.RT->IsDepthStencilBuffer())
                        {
                            uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
                        }
                        else
                        {
                            uavDesc.Format = (DXGI_FORMAT)resourceDesc.RT->GetFormat();
                        }
                        
                        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                        device->CreateUnorderedAccessView(defaultResourceBuffer, nullptr, &uavDesc, handle);
                        
                        break;
                    }
                default:
                    break;
                }

                if(resourceDesc.Type == RTYPE_Sampler)
                {
                    samplerHandle.ptr += samplerDescriptorSize;
                }
                else if(resourceDesc.Type == RTYPE_Constant)
                {
                    ResourcesMap[resourceDesc.Name] = new ResourceData{ NULL, NULL, resourceDesc };
                }
                else
                {
                    uploadResourceBuffer->SetName(DX12Helper::StringToLPCWSTR(resourceDesc.Name + "_Upload").c_str());
                    defaultResourceBuffer->SetName(DX12Helper::StringToLPCWSTR(resourceDesc.Name + "_Default").c_str());
                    ResourcesMap[resourceDesc.Name] = new ResourceData{ uploadResourceBuffer, defaultResourceBuffer, resourceDesc };

                    if(resourceDesc.Data)
                    {
                        //map and copy data
                        UINT8* pMappedData;
                        hr = uploadResourceBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
                        if(FAILED(hr))
                        {
                            DX12Helper::PrintHResultError(hr);
                        }
                        memcpy(pMappedData, resourceDesc.Data, size);
                        uploadResourceBuffer->Unmap(0, nullptr);
                        
                        cmdList->ResourceBarrier(defaultResourceBuffer, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
                        cmdList->CopyResource(defaultResourceBuffer, uploadResourceBuffer);
                        cmdList->ResourceBarrier(defaultResourceBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    }

                    handle.ptr += descriptorSize;
                }
            }
        }
    }

    void DX12RootSignature::UpdateResourceData(String name, void* data)
    {
        if(data)
        {
            auto resourceData = ResourcesMap[name];
            
            if(resourceData)
            {
                if(resourceData->Desc.Type == RTYPE_Constant)
                {
                    uint32_t numConstants = resourceData->Desc.Size.x / resourceData->Desc.Stride;
                    CmdList->SetConstants(resourceData->Desc.Slot, numConstants, data);
                }
                else
                {
                    //map and copy data
                    UINT8* pMappedData;
                    HRESULT hr = resourceData->DX12UploadResource->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
                    if(FAILED(hr))
                    {
                        DX12Helper::PrintHResultError(hr);
                    }
                    memcpy(pMappedData, data, resourceData->Desc.Size.x * resourceData->Desc.Size.y);
                    resourceData->DX12UploadResource->Unmap(0, nullptr);

                    //copy data to default resource
                    //barrier
                    CmdList->ResourceBarrier(resourceData->DX12DefaultResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
                    CmdList->CopyResource(resourceData->DX12DefaultResource, resourceData->DX12UploadResource);
                    CmdList->ResourceBarrier(resourceData->DX12DefaultResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                }
            }
            else
            {
                throw std::runtime_error("Resource not found!");
            }
        }
    }
}