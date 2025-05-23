#pragma once
#include "GraphicTypes.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    enum GraphicResourceType
    {
        RTYPE_ConstantBuffer = 0,
        RTYPE_Buffer = 1,
        RTYPE_BufferRaw = 2,
        RTYPE_RWBuffer = 3,
        RTYPE_RWBufferRaw = 4,
        RTYPE_Texture = 5,
        RTYPE_RWTexture = 6,
        RTYPE_Sampler = 7,
        RTYPE_RenderTarget = 8,
        RTYPE_RWRenderTarget = 9,
        RTYPE_AccelerationStructure = 10,
        RTYPE_Constant = 19
    };

    enum ResourceHeapType
    {
        SRV_UAV_CBV = 0,
        RTV = 1,
    };
    
    class WALDEM_API GraphicResource
    {
        GraphicResourceType Type = RTYPE_Texture;
        uint64 GPUAddress = 0;
        WMap<ResourceHeapType, uint> GPUIndexMap;
        ResourceStates CurrentState = COMMON;
        GraphicResource* UploadResource = nullptr;
        GraphicResource* ReadbackResource = nullptr;

    public:
        uint64 GetGPUAddress() const { return GPUAddress; }

        uint GetIndex(ResourceHeapType heapType) { return GPUIndexMap[heapType]; }
        
        GraphicResourceType GetType() const { return Type; }

        ResourceStates GetCurrentState() const { return CurrentState; }
        
        GraphicResource* GetUploadResource() { return UploadResource; }
        
        GraphicResource* GetReadbackResource() { return ReadbackResource; }

        void SetGPUAddress(uint64 address) { GPUAddress = address; }
        
        void SetIndex(uint index, ResourceHeapType heapType) { GPUIndexMap[heapType] = index; }
        
        void SetType(GraphicResourceType type) { Type = type; }

        void SetCurrentState(ResourceStates state) { CurrentState = state; }
        
        void SetUploadResource(GraphicResource* uploadResource) { UploadResource = uploadResource; }
        
        void SetReadbackResource(GraphicResource* readbackResource) { ReadbackResource = readbackResource; }
    };
}
