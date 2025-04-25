#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/RootSignature.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    struct ShaderBindingTable
    {
        ID3D12Resource* Resource;
        uint64_t Size;
    };
    
    class WALDEM_API DX12RayTracingPipeline : public Pipeline
    {
    public:
        void BuildShaderTables(ID3D12Device5* device, LPCWSTR RayGenShaderName, LPCWSTR MissShaderName,
                               LPCWSTR HitGroupName);
        DX12RayTracingPipeline(const WString& name, ID3D12Device5* device, RootSignature* rootSignature, RayTracingShader* shader);
        ~DX12RayTracingPipeline() override;
        void* GetNativeObject() const override { return NativePipeline; }
        ShaderBindingTable* GetRayGenSBT() const { return RayGenSBT; }
        ShaderBindingTable* GetMissSBT() const { return MissSBT; }
        ShaderBindingTable* GetHitGroupSBT() const { return HitGroupSBT; }
        D3D12_STATE_OBJECT_DESC* GetDesc() { return &Desc; }
        void Destroy() override { NativePipeline->Release(); RayGenSBT->Resource->Release(); MissSBT->Resource->Release(); HitGroupSBT->Resource->Release(); }

    private:
        ID3D12StateObject* NativePipeline;
        ShaderBindingTable* RayGenSBT;
        ShaderBindingTable* MissSBT;
        ShaderBindingTable* HitGroupSBT;
        D3D12_STATE_OBJECT_DESC Desc;
    };
}
