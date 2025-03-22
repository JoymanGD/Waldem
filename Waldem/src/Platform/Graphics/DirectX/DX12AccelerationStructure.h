#pragma once
#include <d3d12.h>

#include "DX12CommandList.h"
#include "Waldem/Renderer/AccelerationStructure.h"

class RayTracingGeometry;

namespace Waldem
{
    class WALDEM_API DX12AccelerationStructure : public AccelerationStructure
    {
    public:
        DX12AccelerationStructure(String name, ID3D12Device5* device, DX12CommandList* cmdList, AccelerationStructureType type, WArray<RayTracingGeometry>& geometries);
        DX12AccelerationStructure(String name, ID3D12Device5* device, DX12CommandList* cmdList, AccelerationStructureType type, WArray<RayTracingInstance>& instances);
        void* GetPlatformResource() override { return Resource; }

    private:
        ID3D12Resource* Resource;
    };
}
