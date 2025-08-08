#pragma once
#include "Renderer.h"
#include "ResizableBuffer.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    class WALDEM_API ResizableAccelerationStructure
    {
    public:
        uint ThresholdNum = 0;

        ResizableAccelerationStructure() = default;

        ResizableAccelerationStructure(const WString& name, uint thresholdNum) : ThresholdNum(thresholdNum)
        {
            InstanceBuffer = new ResizableBuffer(name + "_InstanceBuffer", StorageBuffer, sizeof(RayTracingInstance), thresholdNum);
            TLAS = Renderer::CreateTLAS(name, InstanceBuffer->GetBuffer(), Num());

            WArray DummyVertexData
            {
                Vertex(Vector3(0, 0, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(1, 0, 0), Vector3(0, 1, 0), Vector2(0, 0), 0),
                Vertex(Vector3(0, 2, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(1, 0, 0), Vector3(0, 1, 0), Vector2(0, 1), 0),
                Vertex(Vector3(2, 2, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(1, 0, 0), Vector3(0, 1, 0), Vector2(1, 1), 0),
            };
            WArray DummyIndexData { 0, 1, 2 };

            DummyVertexBuffer = Renderer::CreateBuffer("DummyVertexBuffer", BufferType::VertexBuffer, DummyVertexData.GetSize(), sizeof(Vertex), DummyVertexData.GetData());
            DummyIndexBuffer = Renderer::CreateBuffer("DummyIndexBuffer", BufferType::IndexBuffer, DummyIndexData.GetSize(), sizeof(uint), DummyIndexData.GetData());
            WArray dummyGeometry { RayTracingGeometry(DummyVertexBuffer, DummyIndexBuffer) };
            DummyBLAS = Renderer::CreateBLAS("DummyBLAS", dummyGeometry);
        }

        int AddEmptyData()
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
        
        void SetData(MeshComponent& meshComponent, const Transform& transform)
        {
            WArray geometries { RayTracingGeometry(meshComponent.MeshRef.Mesh->VertexBuffer, meshComponent.MeshRef.Mesh->IndexBuffer) };

            AccelerationStructure* blas = Renderer::CreateBLAS(meshComponent.MeshRef.Mesh->Name, geometries);
            
            BLAS[meshComponent.RTXInstanceId] = blas;
            
            auto& instance = Instances[meshComponent.RTXInstanceId];
            instance.InstanceID = meshComponent.RTXInstanceId;
            instance.InstanceMask = 0xFF;
            instance.InstanceContributionToHitGroupIndex = 0;
            instance.Flags = 0;
            instance.AccelerationStructure = blas->GetGPUAddress();
            auto transposedMatrix = transpose(transform.Matrix);
            memcpy(instance.Transform, &transposedMatrix, sizeof(Matrix3x4));
            
            InstanceBuffer->UpdateData(&instance, sizeof(RayTracingInstance), meshComponent.RTXInstanceId * sizeof(RayTracingInstance));
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        }

        void UpdateTransform(int meshId, Transform& transform)
        {
            auto transposedMatrix = transpose(transform.Matrix);
            InstanceBuffer->UpdateData(&transposedMatrix, sizeof(Matrix3x4), meshId * sizeof(RayTracingInstance));
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        }

        void UpdateGeometry(MeshComponent& meshComponent)
        {
            WArray geometries { RayTracingGeometry(meshComponent.MeshRef.Mesh->VertexBuffer, meshComponent.MeshRef.Mesh->IndexBuffer) };
            
            Renderer::UpdateBLAS(BLAS[meshComponent.RTXInstanceId], geometries);
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        }

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
