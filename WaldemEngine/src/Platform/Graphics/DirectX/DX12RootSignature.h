#pragma once
#include <d3d12.h>
#include <d3dcommon.h>

#include "DX12CommandList.h"
#include "DX12Resource.h"
#include "Waldem/Renderer/GraphicResource.h"
#include "Waldem/Renderer/RootSignature.h"

#include "Waldem/Types/WMap.h"

namespace Waldem
{
    struct RootParamData
    {
        GraphicResourceType Type;
        uint32_t NumDescriptors;
    };
    
    class WALDEM_API DX12RootSignature : public RootSignature
    {
    public:
        DX12RootSignature(ID3D12Device* device, DX12CommandList* cmdList, WArray<GraphicResource> resources);
        ~DX12RootSignature() override;
        void* GetNativeObject() const override { return NativeRootSignature; }
        void UpdateResourceData(WString name, void* data) override;
        void ClearResource(WString name) override;
        void ReadbackResourceData(WString name, void* destinationData) override;
        ID3D12DescriptorHeap* GetResourcesHeap() const { return ResourcesHeap; }
        ID3D12DescriptorHeap* GetSamplersHeap() const { return SamplersHeap; }
        WArray<RootParamData> GetRootParamDatas() { return RootParamDatas; }
        
    private:
        void SetResources(ID3D12Device* device, DX12CommandList* cmdList, WArray<GraphicResource> resourceDescs, uint32_t numDescriptors);

    public:
        void Destroy() override;

    private:
        DX12CommandList* CmdList = nullptr;
        ID3D12RootSignature* NativeRootSignature = nullptr;
        WArray<RootParamData> RootParamDatas;
        WMap<WString, ResourceData*> ResourcesMap;
        ID3D12DescriptorHeap* ResourcesHeap = nullptr;
        ID3D12DescriptorHeap* SamplersHeap = nullptr;
    };
}
