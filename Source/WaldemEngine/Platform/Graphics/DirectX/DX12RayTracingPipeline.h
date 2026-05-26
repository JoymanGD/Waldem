#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Pipeline.h"
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
        DX12RayTracingPipeline(const WString& name, ID3D12Device5* device, ID3D12RootSignature* rootSignature, RayTracingShader* shader);
        ~DX12RayTracingPipeline() override;
        bool Reload() override;
        void* GetNativeObject() const override { return NativePipeline; }
        ShaderBindingTable* GetRayGenSBT() const { return RayGenSBT; }
        ShaderBindingTable* GetMissSBT() const { return MissSBT; }
        ShaderBindingTable* GetHitGroupSBT() const { return HitGroupSBT; }
        D3D12_STATE_OBJECT_DESC* GetDesc() { return &Desc; }
        void Destroy() override;

    private:
        ID3D12Device5* Device = nullptr;
        ID3D12RootSignature* RootSignature = nullptr;
        RayTracingShader* ShaderObject = nullptr;
        ID3D12StateObject* NativePipeline = nullptr;
        ShaderBindingTable* RayGenSBT = nullptr;
        ShaderBindingTable* MissSBT = nullptr;
        ShaderBindingTable* HitGroupSBT = nullptr;
        D3D12_STATE_OBJECT_DESC Desc;
    };
}
