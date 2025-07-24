#pragma once
#include "Renderer.h"
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
        }

        void AddData(MeshComponent& meshComponent, Transform& transform)
        {
            WArray geometries { RayTracingGeometry(meshComponent.Mesh->VertexBuffer, meshComponent.Mesh->IndexBuffer) };

            AccelerationStructure* blas = Renderer::CreateBLAS(meshComponent.Mesh->Name, geometries);
            
            BLAS.Add(blas);
            RayTracingInstance instance;
            instance.InstanceID = Num();
            instance.InstanceMask = 0xFF;
            instance.InstanceContributionToHitGroupIndex = 0;
            instance.Flags = 0;
            instance.AccelerationStructure = blas->GetGPUAddress();
            auto transposedMatrix = transpose(transform.Matrix);
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
        }

        void UpdateTransform(int meshId, Transform& transform)
        {
            InstanceBuffer->UpdateData(&transform, sizeof(Matrix3x4), meshId * sizeof(RayTracingInstance));
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        }

        void UpdateGeometry(int meshId, MeshComponent& meshComponent)
        {
            WArray geometries { RayTracingGeometry(meshComponent.Mesh->VertexBuffer, meshComponent.Mesh->IndexBuffer) };
            
            Renderer::UpdateBLAS(BLAS[meshId], geometries);
            Renderer::UpdateTLAS(TLAS, InstanceBuffer->GetBuffer(), Num());
        }

        operator AccelerationStructure*() const { return TLAS; }
        
        uint64 GetGPUAddress() const { return TLAS->GetGPUAddress(); }
        uint GetIndex(ResourceHeapType heapType) { return TLAS->GetIndex(heapType); }
        
    private:
        AccelerationStructure* TLAS = nullptr;
        WArray<AccelerationStructure*> BLAS;
        ResizableBuffer* InstanceBuffer;
        WArray<RayTracingInstance> Instances;
        int Num() const { return Instances.Num(); }
    };
}
