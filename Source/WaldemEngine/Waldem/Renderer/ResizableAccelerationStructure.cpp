#include "wdpch.h"
#include "Renderer.h"
#include "ResizableBuffer.h"
#include "ResizableAccelerationStructure.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    static RayTracingInstance CreateEmptyInstance(const int id, const uint64 fallbackBLASAddress)
    {
        Matrix4 identityMatrix = Matrix4(1.0f);

        RayTracingInstance instance;
        instance.InstanceID = id;
        instance.InstanceMask = 0;
        instance.InstanceContributionToHitGroupIndex = 0;
        instance.Flags = 0;
        instance.AccelerationStructure = fallbackBLASAddress;

        auto transposedMatrix = transpose(identityMatrix);
        memcpy(instance.Transform, &transposedMatrix, sizeof(instance.Transform));

        return instance;
    }

    ResizableAccelerationStructure::ResizableAccelerationStructure(const WString& name, uint thresholdNum) : ThresholdNum(thresholdNum)
    {
        InstanceBuffer = new ResizableBuffer(name + "_InstanceBuffer", StorageBuffer, sizeof(RayTracingInstance), thresholdNum);
        TLAS = Renderer::CreateTLAS(name, InstanceBuffer->GetBuffer(), thresholdNum);

        WArray DummyVertexData
        {
            Vertex(Vector4(0, 0, 0, 1), Vector4(1, 1, 1, 1), Vector4(0, 0, 1, 0), Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector2(0, 0)),
            Vertex(Vector4(0, 2, 0, 1), Vector4(1, 1, 1, 1), Vector4(0, 0, 1, 0), Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector2(0, 1)),
            Vertex(Vector4(2, 2, 0, 1), Vector4(1, 1, 1, 1), Vector4(0, 0, 1, 0), Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector2(1, 1)),
        };
        WArray DummyIndexData { 0, 1, 2 };

        DummyVertexBuffer = Renderer::CreateBuffer("DummyVertexBuffer", BufferType::VertexBuffer, DummyVertexData.GetSize(), sizeof(Vertex), DummyVertexData.GetData());
        DummyIndexBuffer = Renderer::CreateBuffer("DummyIndexBuffer", BufferType::IndexBuffer, DummyIndexData.GetSize(), sizeof(uint), DummyIndexData.GetData());
        WArray dummyGeometry { RayTracingGeometry(DummyVertexBuffer, DummyIndexBuffer, { 3, 1, 0, 0, 0 }, 3) };
        DummyBLAS = Renderer::CreateBLAS("DummyBLAS", dummyGeometry);
    }

    void ResizableAccelerationStructure::EnsureInstanceSlot(const int id)
    {
        if (id < 0)
        {
            return;
        }

        if (BLAS.Num() <= (size_t)id)
        {
            BLAS.Resize(id + 1, nullptr);
        }

        const size_t previousInstancesCount = Instances.Num();
        if (previousInstancesCount <= (size_t)id)
        {
            Instances.Resize(id + 1);

            const uint64 fallbackBLASAddress = DummyBLAS ? DummyBLAS->GetGPUAddress() : 0ull;
            for (size_t i = previousInstancesCount; i < Instances.Num(); ++i)
            {
                const RayTracingInstance emptyInstance = CreateEmptyInstance((int)i, fallbackBLASAddress);
                Instances[i] = emptyInstance;
                InstanceBuffer->UpdateOrAdd(&Instances[i], sizeof(RayTracingInstance), (uint)(i * sizeof(RayTracingInstance)));
            }
        }
    }

    int ResizableAccelerationStructure::GetBuildInstanceCount() const
    {
        int lastActive = -1;

        for (int i = (int)BLAS.Num() - 1; i >= 0; --i)
        {
            if (BLAS[i] != nullptr)
            {
                lastActive = i;
                break;
            }
        }

        if (lastActive >= 0)
        {
            return lastActive + 1;
        }

        return Instances.Num() > 0 ? 1 : 0;
    }

    int ResizableAccelerationStructure::AddEmptyData(int id)
    {
        if (id < 0)
        {
            id = (int)Instances.Num();
        }

        EnsureInstanceSlot(id);

        const RayTracingInstance emptyInstance = CreateEmptyInstance(id, DummyBLAS ? DummyBLAS->GetGPUAddress() : 0ull);
        Instances[id] = emptyInstance;
        BLAS[id] = nullptr;
        InstanceBuffer->UpdateOrAdd(&Instances[id], sizeof(RayTracingInstance), id * sizeof(RayTracingInstance));

        const int instanceCount = GetBuildInstanceCount();
        if (instanceCount > 0)
        {
            Renderer::BuildTLAS(InstanceBuffer->GetBuffer(), instanceCount, TLAS);
        }

        return id;
    }
    
    void ResizableAccelerationStructure::SetData(int id, WString& name, Buffer* VertexBuffer, Buffer* IndexBuffer, DrawIndexedCommand drawCommand, uint vertexCount, const Transform& transform)
    {
        EnsureInstanceSlot(id);

        WArray geometries { RayTracingGeometry(VertexBuffer, IndexBuffer, drawCommand, vertexCount) };

        AccelerationStructure* blas = Renderer::CreateBLAS(name, geometries);
        
        BLAS[id] = blas;
        
        auto& instance = Instances[id];
        instance.InstanceID = id;
        instance.InstanceMask = 0xFF;
        instance.InstanceContributionToHitGroupIndex = 0;
        instance.Flags = 0;
        instance.AccelerationStructure = blas->GetGPUAddress();
        auto transposedMatrix = transpose(transform.Matrix);
        memcpy(instance.Transform, &transposedMatrix, sizeof(Matrix3x4));
        
        InstanceBuffer->UpdateOrAdd(&instance, sizeof(RayTracingInstance), id * sizeof(RayTracingInstance));

        const int instanceCount = GetBuildInstanceCount();
        if (instanceCount > 0)
        {
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), instanceCount);
        }
    }
    
    void ResizableAccelerationStructure::RemoveData(int id)
    {
        if (id < 0 || id >= (int)BLAS.Num())
        {
            return;
        }

        if(BLAS[id])
        {
            Renderer::Destroy(BLAS[id]);
            BLAS[id] = nullptr;
        }
        
        auto& instance = Instances[id];
        instance.InstanceID = id;
        instance.InstanceMask = 0;
        instance.InstanceContributionToHitGroupIndex = 0;
        instance.Flags = 0;
        instance.AccelerationStructure = DummyBLAS ? DummyBLAS->GetGPUAddress() : 0ull;
        
        InstanceBuffer->UpdateOrAdd(&instance, sizeof(RayTracingInstance), id * sizeof(RayTracingInstance));

        const int instanceCount = GetBuildInstanceCount();
        if (instanceCount > 0)
        {
            Renderer::BuildTLAS(InstanceBuffer->GetBuffer(), instanceCount, TLAS);
        }
    }

    void ResizableAccelerationStructure::UpdateTransform(int id, Transform& transform)
    {
        if (id < 0 || id >= (int)BLAS.Num() || BLAS[id] == nullptr)
        {
            return;
        }

        auto transposedMatrix = transpose(transform.Matrix);
        InstanceBuffer->UpdateData(&transposedMatrix, sizeof(Matrix3x4), id * sizeof(RayTracingInstance));

        const int instanceCount = GetBuildInstanceCount();
        if (instanceCount > 0)
        {
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), instanceCount);
        }
    }

    void ResizableAccelerationStructure::UpdateGeometry(int id, Buffer* vertexBuffer, Buffer* indexBuffer, DrawIndexedCommand drawCommand, uint vertexCount)
    {
        if (id < 0 || id >= (int)BLAS.Num() || BLAS[id] == nullptr)
        {
            return;
        }

        WArray geometries { RayTracingGeometry(vertexBuffer, indexBuffer, drawCommand, vertexCount) };
        
        Renderer::UpdateBLAS(BLAS[id], geometries);

        const int instanceCount = GetBuildInstanceCount();
        if (instanceCount > 0)
        {
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), instanceCount);
        }
    }
}
