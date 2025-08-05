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
            TLAS = Renderer::CreateTLAS(name, InstanceBuffer->GetBuffer(), thresholdNum);

            WArray DummyVertexData
            {
                Vertex(Vector3(0, 0, 0), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
                Vertex(Vector3(0, 2, 0), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
                Vertex(Vector3(2, 2, 0), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
            };
            WArray DummyIndexData { 0, 1, 2 };

            auto dummyVertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, DummyVertexData.GetSize(), sizeof(Vertex), DummyVertexData.GetData());
            auto dummyIndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, DummyIndexData.GetSize(), sizeof(uint), DummyIndexData.GetData());
            WArray dummyGeometry { RayTracingGeometry(dummyVertexBuffer, dummyIndexBuffer) };
            DummyBLAS = Renderer::CreateBLAS("DummyBLAS", dummyGeometry);
        }

        int AddEmptyData()
        {
            BLAS.Add(nullptr);
            
            Matrix4 identityMatrix = Matrix4(1.0f);
            
            RayTracingInstance instance;
            instance.InstanceID = Num();
            instance.InstanceMask = 0xFF;
            instance.InstanceContributionToHitGroupIndex = 0;
            instance.Flags = 0;
            instance.AccelerationStructure = DummyBLAS->GetGPUAddress();
            auto transposedMatrix = transpose(identityMatrix);
            memcpy(instance.Transform, &transposedMatrix, sizeof(instance.Transform));
                
            InstanceBuffer->AddData(&instance, sizeof(RayTracingInstance));

            Instances.Add(instance);
                
            if (Num() <= ThresholdNum)
            {
                Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
            }
            else
            {
                ThresholdNum += ThresholdNum;

                Renderer::Destroy(TLAS);
                Renderer::InitializeTLAS("RayTracingTLAS", InstanceBuffer->GetBuffer(), ThresholdNum, TLAS);
            }

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
            memcpy(instance.Transform, &transposedMatrix, sizeof(instance.Transform));
            
            InstanceBuffer->UpdateData(&instance, sizeof(RayTracingInstance), meshComponent.RTXInstanceId * sizeof(RayTracingInstance));
        }

        void UpdateTransform(int meshId, Transform& transform)
        {
            InstanceBuffer->UpdateData(&transform, sizeof(Matrix3x4), meshId * sizeof(RayTracingInstance));
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        }

        void UpdateGeometry(MeshComponent& meshComponent)
        {
            WArray geometries { RayTracingGeometry(((CMesh*)meshComponent.MeshRef.Mesh)->VertexBuffer, ((CMesh*)meshComponent.MeshRef.Mesh)->IndexBuffer) };
            
            Renderer::UpdateBLAS(BLAS[meshComponent.DrawId], geometries);
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        }

        operator AccelerationStructure*() const { return TLAS; }
        
        uint64 GetGPUAddress() const { return TLAS->GetGPUAddress(); }
        uint GetIndex(ResourceHeapType heapType) { return TLAS->GetIndex(heapType); }
        
    private:
        AccelerationStructure* TLAS = nullptr;
        AccelerationStructure* DummyBLAS = nullptr;
        WArray<AccelerationStructure*> BLAS;
        ResizableBuffer* InstanceBuffer;
        WArray<RayTracingInstance> Instances;
        int Num() const { return Instances.Num(); }
    };
}
