#pragma once
#include <d3d12.h>
#include <d3dcommon.h>

#include "DX12GraphicCommandList.h"
#include "DX12Resource.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/RootSignature.h"

namespace Waldem
{
    class WALDEM_API DX12RootSignature : public RootSignature 
    {
    public:
        DX12RootSignature(ID3D12Device* device, DX12GraphicCommandList* cmdList, WArray<Resource> resources);
        ~DX12RootSignature() override {}
        void* GetNativeObject() const override { return NativeRootSignature; }
        
    private:
        void SetResources(ID3D12Device* device, DX12GraphicCommandList* cmdList, WArray<Resource> resourceDescs, uint32_t numDescriptors);
        
        ID3D12RootSignature* NativeRootSignature;
        WArray<ResourceType> RootParamTypes;
        std::unordered_map<String, ResourceData*> Resources;
        uint32_t InitializedDescriptorsAmount = 0;
        ID3D12DescriptorHeap* ResourcesHeap;
        ID3D12DescriptorHeap* SamplersHeap;
    };
}
