#pragma once
#include "ResizableBuffer.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    struct MeshComponent;

    class WALDEM_API ResizableAccelerationStructure
    {
    public:
        uint ThresholdNum = 0;

        ResizableAccelerationStructure() = default;

        ResizableAccelerationStructure(const WString& name, uint thresholdNum);

        int AddEmptyData();
        
        void SetData(int id, WString& name, Buffer* VertexBuffer, Buffer* IndexBuffer, DrawCommand drawCommand, uint vertexCount, const Transform& transform);
        
        void RemoveData(int id);

        void UpdateTransform(int id, Transform& transform);

        void UpdateGeometry(int id, Buffer* vertexBuffer, Buffer* indexBuffer, DrawCommand drawCommand, uint vertexCount);
        
        operator AccelerationStructure*() const { return TLAS; }
        
        uint64 GetGPUAddress() const { return TLAS->GetGPUAddress(); }
        uint GetIndex(ResourceHeapType heapType) { return TLAS->GetIndex(heapType); }
        
    private:
        AccelerationStructure* TLAS = nullptr;
        AccelerationStructure* DummyBLAS = nullptr;
        Buffer* DummyVertexBuffer;
        Buffer* DummyIndexBuffer;
        WArray<AccelerationStructure*> BLAS;
        ResizableBuffer* InstanceBuffer;
        WArray<RayTracingInstance> Instances;
        int Num() const { return Instances.Num(); }
    };
}
