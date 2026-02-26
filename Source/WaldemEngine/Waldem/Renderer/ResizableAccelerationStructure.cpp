#include "wdpch.h"
#include "Renderer.h"
#include "ResizableBuffer.h"
#include "ResizableAccelerationStructure.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
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

    int ResizableAccelerationStructure::AddEmptyData()
    {
        BLAS.Add(nullptr);
        
        Matrix4 identityMatrix = Matrix4(1.0f);
        
        RayTracingInstance instance;
        instance.InstanceID = Num();
        instance.InstanceMask = 0;
        instance.InstanceContributionToHitGroupIndex = 0;
        instance.Flags = 0;
        instance.AccelerationStructure = DummyBLAS->GetGPUAddress();
        auto transposedMatrix = transpose(identityMatrix);
        memcpy(instance.Transform, &transposedMatrix, sizeof(instance.Transform));
            
        InstanceBuffer->AddData(&instance, sizeof(RayTracingInstance));

        Instances.Add(instance);
            
        // if (Num() <= ThresholdNum)
        // {
        //     Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        // }
        // else
        // {
            // ThresholdNum += ThresholdNum;

            // Renderer::Destroy(TLAS);
            // Renderer::InitializeTLAS("RayTracingTLAS", InstanceBuffer->GetBuffer(), Num(), TLAS);
        // }
        
        Renderer::BuildTLAS(InstanceBuffer->GetBuffer(), Num(), TLAS);

        return instance.InstanceID;
    }
    
    void ResizableAccelerationStructure::SetData(int id, WString& name, Buffer* VertexBuffer, Buffer* IndexBuffer, DrawIndexedCommand drawCommand, uint vertexCount, const Transform& transform)
    {
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
        
        InstanceBuffer->UpdateData(&instance, sizeof(RayTracingInstance), id * sizeof(RayTracingInstance));
        Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
    }
    
    void ResizableAccelerationStructure::RemoveData(int id)
    {
        if(BLAS[id])
        {
            Renderer::Destroy(BLAS[id]);
        }
        
        auto& instance = Instances[id];
        instance.InstanceID = 0;
        instance.InstanceMask = 0;
        instance.InstanceContributionToHitGroupIndex = 0;
        instance.Flags = 0;
        instance.AccelerationStructure = -1;
        
        InstanceBuffer->RemoveData(sizeof(RayTracingInstance), id * sizeof(RayTracingInstance));
        
        Renderer::BuildTLAS(InstanceBuffer->GetBuffer(), Num(), TLAS);
    }

    void ResizableAccelerationStructure::UpdateTransform(int id, Transform& transform)
    {
        auto transposedMatrix = transpose(transform.Matrix);
        InstanceBuffer->UpdateData(&transposedMatrix, sizeof(Matrix3x4), id * sizeof(RayTracingInstance));
        Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
    }

    void ResizableAccelerationStructure::UpdateGeometry(int id, Buffer* vertexBuffer, Buffer* indexBuffer, DrawIndexedCommand drawCommand, uint vertexCount)
    {
        WArray geometries { RayTracingGeometry(vertexBuffer, indexBuffer, drawCommand, vertexCount) };
        
        Renderer::UpdateBLAS(BLAS[id], geometries);
        Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
    }
}
