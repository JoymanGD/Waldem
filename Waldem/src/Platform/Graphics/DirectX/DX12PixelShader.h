#pragma once
#include <d3d12.h>
#include <d3dcommon.h>
#include "DX12CommandList.h"
#include "Waldem/Renderer/RenderTarget.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/Renderer/Shader.h"

#define MAX_CBV_PER_ROOT_SIGNATURE 14
#define MAX_SRV_PER_ROOT_SIGNATURE 128
#define MAX_UAV_PER_ROOT_SIGNATURE 64
#define MAX_SAMPLER_PER_ROOT_SIGNATURE 14

namespace Waldem
{
    struct ResourceData
    {
        ID3D12Resource* DX12Resource;
        Resource Desc;
    };
    
    class WALDEM_API DX12PixelShader : public PixelShader
    {
    public:
        DX12PixelShader(const String& name, ID3D12Device* device, DX12CommandList* cmdList, std::vector<Resource> resources, Waldem::RenderTarget* renderTarget = nullptr);
        ~DX12PixelShader() override;

        ID3D12PipelineState* GetPipeline() const { return PipelineState; }
        ID3D12RootSignature* GetRootSignature() const { return RootSignature; }
        ID3D12DescriptorHeap* GetResourcesHeap() const { return ResourcesHeap; }
        ID3D12DescriptorHeap* GetSamplersHeap() const { return SamplersHeap; }

    private:
        bool CompileFromFile(const String& filepath);
        void SetResources(std::vector<Resource> resourceDescs, uint32_t numDescriptors);
        void SetSamplers(std::vector<SamplerData> samplers) override;

    public:
        void UpdateResourceData(String name, void* data) override;
        std::vector<D3D12_ROOT_PARAMETER> GetRootParams() const { return RootParams; }
        uint32_t GetInitializedDescriptorsAmount() const { return InitializedDescriptorsAmount; }

    private:
        ID3DBlob* VertexShaderBlob;
        ID3DBlob* PixelShaderBlob;
        ID3DBlob* ErrorBlob;
        ID3D12PipelineState* PipelineState;
        ID3D12RootSignature* RootSignature;
        ID3D12DescriptorHeap* ResourcesHeap;
        ID3D12DescriptorHeap* SamplersHeap;
        ID3D12Device* Device;
        DX12CommandList* CmdList;
        std::vector<D3D12_ROOT_PARAMETER> RootParams;

        std::unordered_map<String, ResourceData*> Resources;
        uint32_t InitializedDescriptorsAmount = 0;
    };
}
