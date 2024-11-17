#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "DX12GraphicCommandList.h"
#include "DX12Resource.h"
#include "DX12Shader.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12ComputeShader : public ComputeShader
    {
    public:
        DX12ComputeShader(const String& name, ID3D12Device* device, DX12GraphicCommandList* cmdList, WArray<Resource> resources);
        ~DX12ComputeShader() override;

        ID3D12PipelineState* GetPipeline() const { return PipelineState; }
        ID3D12RootSignature* GetRootSignature() const { return RootSignature; }
        ID3D12DescriptorHeap* GetResourcesHeap() const { return ResourcesHeap; }
        ID3D12DescriptorHeap* GetSamplersHeap() const { return SamplersHeap; }
        std::unordered_map<String, ResourceData*> GetResources() const { return Resources; }

    private:
        bool CompileFromFile(const String& filepath) override;
        void SetResources(WArray<Resource> resourceDescs, uint32_t numDescriptors);

    public:
        void UpdateResourceData(String name, void* data) override;
        WArray<ResourceType> GetRootParamTypes() const { return RootParamTypes; }
        void* GetPlatformData() override { return ShaderBlob; }

    private:
        ID3DBlob* ShaderBlob;
        ID3DBlob* ErrorBlob;
        ID3D12PipelineState* PipelineState;
        ID3D12RootSignature* RootSignature;
        ID3D12DescriptorHeap* ResourcesHeap;
        ID3D12DescriptorHeap* SamplersHeap;
        ID3D12Device* Device;
        DX12GraphicCommandList* CmdList;
        WArray<ResourceType> RootParamTypes;

        std::unordered_map<String, ResourceData*> Resources;
        uint32_t InitializedDescriptorsAmount = 0;
    };
}
