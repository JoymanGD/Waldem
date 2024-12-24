#pragma once
#include <d3d12.h>
#include <d3dcommon.h>

#include "DX12CommandList.h"
#include "DX12Resource.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/RootSignature.h"

#include "Waldem/Types/WMap.h"

namespace Waldem
{
    class WALDEM_API DX12RootSignature : public RootSignature
    {
    public:
        DX12RootSignature(ID3D12Device* device, DX12CommandList* cmdList, WArray<Resource> resources);
        ~DX12RootSignature() override;
        void* GetNativeObject() const override { return NativeRootSignature; }
        void UpdateResourceData(String name, void* data) override;
        void ReadbackResourceData(String name, void* destinationData) override;
        ID3D12DescriptorHeap* GetResourcesHeap() const { return ResourcesHeap; }
        ID3D12DescriptorHeap* GetSamplersHeap() const { return SamplersHeap; }
        WArray<ResourceType> GetRootParamTypes() { return RootParamTypes; }
        WMap<String, ResourceData*>& GetResourcesMap() { return ResourcesMap; }
        
    private:
        void SetResources(ID3D12Device* device, DX12CommandList* cmdList, WArray<Resource> resourceDescs, uint32_t numDescriptors);

        DX12CommandList* CmdList;
        ID3D12RootSignature* NativeRootSignature;
        WArray<ResourceType> RootParamTypes;
        WMap<String, ResourceData*> ResourcesMap;
        uint32_t InitializedDescriptorsAmount = 0;
        ID3D12DescriptorHeap* ResourcesHeap;
        ID3D12DescriptorHeap* SamplersHeap;
    };
}
