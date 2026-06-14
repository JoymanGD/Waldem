#pragma once
#include "Waldem/ECS/IdManager.h"
#include "Waldem/Input/Input.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/SkeletalMeshComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Sky.h"
#include "Waldem/ECS/Components/Sprite.h"
#include "Waldem/Renderer/ResizableAccelerationStructure.h"
#include "Waldem/Renderer/ResizableBuffer.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Coach/TinyCuda/NIV/NIVCoach.h"
#include <cstring>
#include <algorithm>
#include <vector>

#define MAX_INDIRECT_COMMANDS 500

namespace Waldem
{
    struct SGBufferSceneData
    {
        Matrix4 ViewMatrix;
        Matrix4 ProjectionMatrix;
        Matrix4 WorldMatrix;
        Matrix4 InverseProjectionMatrix;
    };

    struct GBufferRootConstants
    {
        uint WorldTransforms;
        uint MaterialAttributes;
        uint SceneDataBuffer;
    };
    
    struct SRayTracingSceneData
    {
        Matrix4 InvViewMatrix;
        Matrix4 InvProjectionMatrix;
        Vector3 CameraPosition;
        int NumLights = 0;
    };

    struct RayTracingRootConstants
    {
        uint WorldPositionRT;
        uint NormalRT;
        uint ColorRT;
        uint ORMRT;
        uint RadianceRT;
        uint PathTracingRT;
        uint ReflectionRT;
        uint LightsBuffer;
        uint LightTransformsBuffer;
        uint LightsIndicesBuffer;
        uint SceneDataBuffer; 
        uint TLAS;
        uint VertexBuffer;
        uint IndexBuffer;
        uint DrawCommandsBuffer;
        uint MaterialBuffer;
        uint EnableReflections;
        uint EnableDirectLighting;
        uint EnableSpecular;
        uint EnableMetallic;
        uint EnablePathTracing;
        uint PathTracingMaxBounces;
        uint PathTracingSamplesPerPixel;
        uint PathTracingFrameIndex;
        uint EnablePathTracingAccumulation;
    };
    
    struct DeferredRootConstants
    {
        uint MeshIDRT;
        uint RadianceRT;
        uint PathTracingRT;
        uint ColorRT;
        uint DeferredRT;
        uint NIVIrradianceRT;
        uint NIVPredictedBuffer;
        uint SkyColorRT;
        uint EnableSky;
        uint EnablePathTracing;
        uint EnableNIVInference;
        uint HasNIVPrediction;
        uint EnableNIVSpatialFilter;
        float NIVSpatialFilterStrength;
    };

    struct NIVPackRootConstants
    {
        uint WorldPositionRT;
        uint WorldNormalRT;
        uint MeshIDRT;
        uint OutWorldPositionBuffer;
        uint OutWorldNormalValidBuffer;
        uint Width;
        uint Height;
    };

    struct SkinningRootConstants
    {
        uint BindPoseVertexBuffer;
        uint SkinnedVertexBuffer;
        uint VertexBonesBuffer;
        uint BoneMatricesBuffer;
        uint VertexOffset;
        uint VertexCount;
    };

    struct SkySceneData
    {
        Matrix4 InverseProjection;
        Matrix4 InverseView;
        Matrix4 ViewProjection;
        Vector4 SkyZenithColor;
        Vector4 SkyHorizonColor;
        Vector4 GroundColor;
        Vector4 SunDirection;
        Vector4 CameraPosition;
    };

    struct SkyRootConstants
    {
        uint SceneDataBuffer;
    };
    
    class WALDEM_API HybridRenderingSystem : public ICoreSystem
    {
        inline static HybridRenderingSystem* ActiveInstance = nullptr;

        //Sky pass
        Pipeline* SkyPipeline = nullptr;
        PixelShader* SkyPixelShader = nullptr;
        Quad FullscreenQuad = {};
        SkyRootConstants SkyPassRootConstants;
        SkySceneData SkyPassSceneData;
        Buffer* SceneDataBuffer = nullptr;
        
        //GBuffer pass
        Pipeline* BFCGBufferPipeline = nullptr; // Back face culling GBuffer pipeline
        Pipeline* NCGBufferPipeline = nullptr; // No culling GBuffer pipeline
        PixelShader* GBufferPixelShader = nullptr;
        ResizableBuffer DrawCommandsBuffer;
        ResizableBuffer BFCIndirectBuffer; //Back face culling indirect buffer
        WArray<IndirectIndexedCommand> BFCIndirectCommands;
        ResizableBuffer NCIndirectBuffer; //No culling indirect buffer
        WArray<IndirectIndexedCommand> NCIndirectCommands;
        ResizableBuffer WorldTransformsBuffer;
        ResizableBuffer MaterialAttributesBuffer;
        Buffer* GBufferSceneDataBuffer = nullptr;
        GBufferRootConstants GBufferRootConstants;
        SGBufferSceneData GBufferSceneData;
        WArray<MaterialShaderAttribute> MaterialAttributes;
        size_t VerticesCount = 0;
        size_t IndicesCount = 0;

        //RayTracing
        Pipeline* RTPipeline = nullptr;
        RayTracingShader* RTShader = nullptr;
        Pipeline* PathTracingPipeline = nullptr;
        RayTracingShader* PathTracingShader = nullptr;
        WArray<AccelerationStructure*> BLAS;
        WMap<StaticMesh*, AccelerationStructure*> BLASToUpdate;
        SRayTracingSceneData RayTracingSceneData;
        ResizableBuffer LightsBuffer;
        ResizableBuffer LightTransformsBuffer;
        ResizableBuffer LightsIndicesBuffer;
        WArray<int> LightsIndices;
        Buffer* RayTracingSceneDataBuffer = nullptr;
        RayTracingRootConstants RayTracingRootConstants;
        WArray<Matrix4> LightTransforms;
        uint PathTracingFrameIndex = 0;
        bool PathTracingHistoryValid = false;
        Matrix4 LastPathTracingInvView = Matrix4(1.0f);
        Matrix4 LastPathTracingInvProjection = Matrix4(1.0f);
        Vector3 LastPathTracingCameraPosition = Vector3(0.0f);

        //Deferred
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        Point3 GroupCount;
        DeferredRootConstants DeferredRootConstants;
        ComputeShader* NIVPackComputeShader = nullptr;
        Pipeline* NIVPackPipeline = nullptr;
        NIVPackRootConstants NIVPackRootConstants;
        Buffer* NIVWorldPositionBuffer = nullptr;    // float4[pixelCount]
        Buffer* NIVWorldNormalValidBuffer = nullptr; // float4[pixelCount]
        Buffer* NIVPredictedOutputBuffer = nullptr; // float4[pixelCount]
        Buffer* NIVPredictedHistoryBuffer = nullptr; // float4[pixelCount]
        uint NIVBufferCapacityPixels = 0;

        //Skeletal mesh skinning
        Pipeline* SkinningPipeline = nullptr;
        ComputeShader* SkinningComputeShader = nullptr;
        SkinningRootConstants SkinningConstants;

        struct SkeletalSkinningEntry
        {
            uint BindPoseVertexSRV;
            uint VertexBonesSRV;
            Buffer* BoneMatricesBuffer;
            int VertexOffset;
            uint VertexCount;
            DrawIndexedCommand DrawCommand;
            int GlobalDrawId;
        };
        WMap<flecs::entity_t, SkeletalSkinningEntry> SkeletalSkinningData;

        //Sprite rendering
        WArray<Vertex> SpriteVertices;
        WArray<uint32> SpriteIndices;
        Buffer* SpriteVertexBuffer;
        Buffer* SpriteIndexBuffer;
        
    public:
        HybridRenderingSystem()
        {
            ActiveInstance = this;
            SpriteVertices =
            {
                { {Vector4(-0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(0,1)} },
                { {Vector4( 0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(1,1)} },
                { {Vector4( 0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(1,0)} },
                { {Vector4(-0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(0,0)} },
            };
            SpriteIndices = { 0,1,2, 2,3,0 };

            SpriteVertexBuffer = Renderer::CreateBuffer("SpriteVertexBuffer", BufferType::VertexBuffer, SpriteVertices.GetSize(), sizeof(Vertex), SpriteVertices.GetData());
            SpriteIndexBuffer = Renderer::CreateBuffer("SpriteIndexBuffer", BufferType::IndexBuffer, SpriteIndices.GetSize(), sizeof(uint32_t), SpriteIndices.GetData());
        }

        static void ResetSceneRuntimeData()
        {
            if(ActiveInstance == nullptr)
            {
                return;
            }

            auto& instance = *ActiveInstance;

            instance.BFCIndirectCommands.Clear();
            instance.NCIndirectCommands.Clear();
            instance.MaterialAttributes.Clear();
            instance.LightsIndices.Clear();
            instance.LightTransforms.Clear();
            instance.SkeletalSkinningData.Clear();

            instance.VerticesCount = 0;
            instance.IndicesCount = 0;
            instance.PathTracingFrameIndex = 0;
            instance.PathTracingHistoryValid = false;

            instance.DrawCommandsBuffer.Size = 0;
            instance.BFCIndirectBuffer.Size = 0;
            instance.NCIndirectBuffer.Size = 0;
            instance.WorldTransformsBuffer.Size = 0;
            instance.MaterialAttributesBuffer.Size = 0;
            instance.LightsBuffer.Size = 0;
            instance.LightTransformsBuffer.Size = 0;
            instance.LightsIndicesBuffer.Size = 0;

            Renderer::RenderData.VertexBuffer.Size = 0;
            Renderer::RenderData.IndexBuffer.Size = 0;
        }
        
        void Initialize() override
        {
            //Sky
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            SceneDataBuffer = Renderer::CreateBuffer("SkySceneDataBuffer", BufferType::StorageBuffer, sizeof(SkyPassSceneData), sizeof(SkyPassSceneData));
            SkyPassRootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_CBV);
            
            SkyPixelShader = Renderer::LoadPixelShader("Sky");
            SkyPipeline = Renderer::CreateGraphicPipeline("SkyPipeline",
                                                            SkyPixelShader,
                                                            { SGBuffer::GetFormat(SkyColor) },
                                                            TextureFormat::UNKNOWN,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            inputElementDescs);
            
            //GBuffer
            DrawCommandsBuffer = ResizableBuffer("DrawCommandsBuffer", BufferType::StorageBuffer, sizeof(DrawIndexedCommand), MAX_INDIRECT_COMMANDS);
            BFCIndirectBuffer = ResizableBuffer("BFCIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectIndexedCommand), MAX_INDIRECT_COMMANDS);
            NCIndirectBuffer = ResizableBuffer("NCIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectIndexedCommand), MAX_INDIRECT_COMMANDS);
            Renderer::RenderData.VertexBuffer = ResizableBuffer("VertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), 1000);
            Renderer::RenderData.IndexBuffer = ResizableBuffer("IndexBuffer", BufferType::IndexBuffer, sizeof(uint), 1000);
            WorldTransformsBuffer = ResizableBuffer("WorldTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), MAX_INDIRECT_COMMANDS);
            MaterialAttributesBuffer = ResizableBuffer("MaterialAttributesBuffer", BufferType::StorageBuffer, sizeof(MaterialShaderAttribute), MAX_INDIRECT_COMMANDS);
            GBufferSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(SGBufferSceneData), sizeof(SGBufferSceneData));
            
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            BFCGBufferPipeline = Renderer::CreateGraphicPipeline("BFCGBufferPipeline",
                                                            GBufferPixelShader,
                                                            { SGBuffer::GetFormat(WorldPosition), SGBuffer::GetFormat(Normal), SGBuffer::GetFormat(Color), SGBuffer::GetFormat(ORM), SGBuffer::GetFormat(MeshID) },
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            RasterizerDesc spriteRasterizer = DEFAULT_RASTERIZER_DESC;
            spriteRasterizer.CullMode = WD_CULL_MODE_NONE;
            NCGBufferPipeline = Renderer::CreateGraphicPipeline("NCGBufferPipeline",
                                                            GBufferPixelShader,
                                                            { SGBuffer::GetFormat(WorldPosition), SGBuffer::GetFormat(Normal), SGBuffer::GetFormat(Color), SGBuffer::GetFormat(ORM), SGBuffer::GetFormat(MeshID) },
                                                            TextureFormat::D32_FLOAT,
                                                            spriteRasterizer,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            //Raytracing
            RTShader = Renderer::LoadRayTracingShader("RayTracing/Radiance");
            RTPipeline = Renderer::CreateRayTracingPipeline("RayTracingPipeline", RTShader);
            PathTracingShader = Renderer::LoadRayTracingShader("RayTracing/PathTracing");
            PathTracingPipeline = Renderer::CreateRayTracingPipeline("PathTracingPipeline", PathTracingShader);
            LightsBuffer = ResizableBuffer("LightsBuffer", StorageBuffer, sizeof(Light), 40);
            LightTransformsBuffer = ResizableBuffer("LightTransformsBuffer", StorageBuffer, sizeof(Matrix4), 40);
            LightsIndicesBuffer = ResizableBuffer("LightsIndicesBuffer", StorageBuffer, sizeof(int), 40);
            RayTracingSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", StorageBuffer, sizeof(SRayTracingSceneData), sizeof(SRayTracingSceneData), &RayTracingSceneData);
            Renderer::RenderData.TLAS = ResizableAccelerationStructure("RayTracingTLAS", 50);
            
            //Deferred
            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingComputeShader);
            NIVPackComputeShader = Renderer::LoadComputeShader("NIVPackInputs");
            NIVPackPipeline = Renderer::CreateComputePipeline("NIVPackPipeline", NIVPackComputeShader);
            NIVWorldPositionBuffer = Renderer::CreateBuffer("NIVWorldPositionBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);

            //Skeletal mesh skinning
            SkinningComputeShader = Renderer::LoadComputeShader("Animation");
            SkinningPipeline = Renderer::CreateComputePipeline("SkinningPipeline", SkinningComputeShader);
            NIVWorldNormalValidBuffer = Renderer::CreateBuffer("NIVWorldNormalValidBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);
            NIVPredictedOutputBuffer = Renderer::CreateBuffer("NIVPredictedOutputBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);
            NIVPredictedHistoryBuffer = Renderer::CreateBuffer("NIVPredictedHistoryBuffer", StorageBuffer, sizeof(float) * 4, sizeof(float) * 4);
            NIVBufferCapacityPixels = 1;
            
            ECS::World.observer<MeshComponent, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, MeshComponent& meshComponent, Transform& transform)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                auto bfcDrawId = IdManager::AddId(entity, BackFaceCullingDrawIdType);
                
                if(bfcDrawId >= BFCIndirectCommands.Num())
                {
                    BFCIndirectCommands.Add(IndirectIndexedCommand());
                    BFCIndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectIndexedCommand), bfcDrawId * sizeof(IndirectIndexedCommand));
                }
                
                WorldTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.AddData(nullptr, sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.AddData(nullptr, sizeof(DrawIndexedCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData(globalDrawId);
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnSet).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int bfcDrawId;

                    if(IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId))
                    {
                        bool meshReferenceIsEmpty = meshComponent.MeshRef.Reference.empty() || meshComponent.MeshRef.Reference == "Empty";
                        
                        if(meshReferenceIsEmpty && !meshComponent.MeshRef.IsValid())
                        {
                            return;
                        }

                        if(!meshReferenceIsEmpty && !meshComponent.MeshRef.IsValid())
                        {
                            meshComponent.MeshRef.LoadAsset();
                        }
                        
                        if(meshComponent.MeshRef.IsValid())
                        {
                            if(meshComponent.MaterialRef.Reference.empty() || meshComponent.MaterialRef.Reference == "Empty")
                            {
                                meshComponent.MaterialRef.Reference = meshComponent.MeshRef.Mesh->MaterialPath;
                                meshComponent.MaterialRef.IsLoaded = false;
                            }

                            if(!meshComponent.MaterialRef.IsLoaded)
                            {
                                meshComponent.MaterialRef.LoadAsset();
                            }

                            Material* activeMaterial = meshComponent.MaterialRef.Mat;
                            uint vertexCount = meshComponent.MeshRef.Mesh->VertexData.Num();

                            const bool isFirstAssignment = (meshComponent.DrawCommand.IndexCountPerInstance == 0);

                            if(isFirstAssignment)
                            {
                                meshComponent.DrawCommand = {
                                    (uint)meshComponent.MeshRef.Mesh->IndexData.Num(),
                                    1,
                                    (uint)IndicesCount,
                                    (int)VerticesCount,
                                    0
                                };

                                auto& command = BFCIndirectCommands[bfcDrawId];
                                command.DrawId = globalDrawId;
                                command.Command = meshComponent.DrawCommand;
                                BFCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * bfcDrawId);

                                Renderer::RenderData.VertexBuffer.AddData(meshComponent.MeshRef.Mesh->VertexData.GetData(), meshComponent.MeshRef.Mesh->VertexData.GetSize());
                                Renderer::RenderData.IndexBuffer.AddData(meshComponent.MeshRef.Mesh->IndexData.GetData(), meshComponent.MeshRef.Mesh->IndexData.GetSize());

                                VerticesCount += vertexCount;
                                IndicesCount += meshComponent.DrawCommand.IndexCountPerInstance;

                                DrawCommandsBuffer.UpdateData(&meshComponent.DrawCommand, sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                                auto& transform = entity.get<Transform>();
                                Renderer::Wait();
                                Renderer::RenderData.TLAS.SetData(globalDrawId, meshComponent.MeshRef.Mesh->Name, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, meshComponent.DrawCommand, vertexCount, transform);
                            }

                            auto& materialAttribute = MaterialAttributes[globalDrawId];

                            if(activeMaterial)
                            {
                                materialAttribute.Albedo = activeMaterial->Albedo;
                                materialAttribute.Metallic = activeMaterial->Metallic;
                                materialAttribute.Roughness = activeMaterial->Roughness;
                                materialAttribute.AlphaCut = activeMaterial->AlphaCut ? 1 : 0;
                                materialAttribute.CastShadows = activeMaterial->CastShadows ? 1 : 0;
                            }
                            materialAttribute.DiffuseTextureID = -1;
                            materialAttribute.NormalTextureID = -1;
                            materialAttribute.ORMTextureID = -1;

                            if(activeMaterial)
                            {
                                if(activeMaterial->HasDiffuseTexture())
                                    materialAttribute.DiffuseTextureID = activeMaterial->GetDiffuseTexture()->GetIndex(SRV_CBV);
                                if(activeMaterial->HasNormalTexture())
                                    materialAttribute.NormalTextureID = activeMaterial->GetNormalTexture()->GetIndex(SRV_CBV);
                                if(activeMaterial->HasORMTexture())
                                    materialAttribute.ORMTextureID = activeMaterial->GetORMTexture()->GetIndex(SRV_CBV);
                            }

                            MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        }

                    }
                }
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int bfcDrawId;
                    
                    if(IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId))
                    {
                        IndirectIndexedCommand& command = BFCIndirectCommands[bfcDrawId];

                        if(meshComponent.MeshRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(meshComponent.MeshRef.Mesh->VertexData.GetSize(), command.Command.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(meshComponent.MeshRef.Mesh->IndexData.GetSize(), command.Command.StartIndexLocation * sizeof(uint));

                            VerticesCount -= meshComponent.MeshRef.Mesh->VertexData.Num();
                            IndicesCount -= meshComponent.MeshRef.Mesh->IndexData.Num();
                        }
                        
                        BFCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), bfcDrawId * sizeof(IndirectIndexedCommand));
                        
                        command.DrawId = -1;
                        command.Command = { 0, 0, 0, 0, 0 };

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);
                        
                        if(entity.has<Transform>())
                        {
                            WorldTransformsBuffer.RemoveData(sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        }

                        IdManager::RemoveId(entity, BackFaceCullingDrawIdType);
                    }

                    IdManager::RemoveId(entity, GlobalDrawIdType);
                }
            });

            ECS::World.observer<SkeletalMeshComponent, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, SkeletalMeshComponent& skeletalMeshComponent, Transform& transform)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                auto bfcDrawId = IdManager::AddId(entity, BackFaceCullingDrawIdType);

                if(bfcDrawId >= BFCIndirectCommands.Num())
                {
                    BFCIndirectCommands.Add(IndirectIndexedCommand());
                    BFCIndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectIndexedCommand), bfcDrawId * sizeof(IndirectIndexedCommand));
                }

                WorldTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.AddData(nullptr, sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.AddData(nullptr, sizeof(DrawIndexedCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData(globalDrawId);
            });

            ECS::World.observer<SkeletalMeshComponent>().event(flecs::OnSet).each([&](flecs::entity entity, SkeletalMeshComponent& skeletalMeshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int bfcDrawId;

                    if(IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId))
                    {
                        bool meshReferenceIsEmpty = skeletalMeshComponent.MeshRef.Reference.empty() || skeletalMeshComponent.MeshRef.Reference == "Empty";

                        if(meshReferenceIsEmpty && !skeletalMeshComponent.MeshRef.IsValid())
                        {
                            return;
                        }

                        if(!meshReferenceIsEmpty && !skeletalMeshComponent.MeshRef.IsValid())
                        {
                            skeletalMeshComponent.MeshRef.LoadAsset();
                        }

                        if(skeletalMeshComponent.MeshRef.IsValid())
                        {
                            if(skeletalMeshComponent.MaterialRef.Reference != skeletalMeshComponent.MeshRef.Mesh->MaterialPath || !skeletalMeshComponent.MaterialRef.IsLoaded)
                            {
                                skeletalMeshComponent.MaterialRef.Reference = skeletalMeshComponent.MeshRef.Mesh->MaterialPath;
                                skeletalMeshComponent.MaterialRef.LoadAsset();
                            }

                            Material* activeMaterial = skeletalMeshComponent.MaterialRef.Mat;
                            SkeletalMesh* skeletalMesh = skeletalMeshComponent.MeshRef.Mesh;

                            skeletalMeshComponent.DrawCommand.IndexCountPerInstance = (uint)skeletalMesh->IndexData.Num();
                            skeletalMeshComponent.DrawCommand.InstanceCount         = 1;
                            skeletalMeshComponent.DrawCommand.StartInstanceLocation = 0;

                            uint vertexCount = skeletalMesh->VertexData.Num();

                            // Only pack vertex/index data on first assignment; re-assignments update in-place
                            bool isFirstAssignment = SkeletalSkinningData.Find(entity.id()) == nullptr;

                            if(isFirstAssignment)
                            {
                                skeletalMeshComponent.DrawCommand.StartIndexLocation  = (uint)IndicesCount;
                                skeletalMeshComponent.DrawCommand.BaseVertexLocation  = (int)VerticesCount;

                                Renderer::RenderData.VertexBuffer.AddData(skeletalMesh->VertexData.GetData(), skeletalMesh->VertexData.GetSize());
                                Renderer::RenderData.IndexBuffer.AddData(skeletalMesh->IndexData.GetData(), skeletalMesh->IndexData.GetSize());

                                VerticesCount += vertexCount;
                                IndicesCount  += skeletalMeshComponent.DrawCommand.IndexCountPerInstance;
                            }

                            auto& command = BFCIndirectCommands[bfcDrawId];
                            command.DrawId = globalDrawId;
                            command.Command = skeletalMeshComponent.DrawCommand;

                            BFCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * bfcDrawId);

                            auto& materialAttribute = MaterialAttributes[globalDrawId];

                            if(activeMaterial)
                            {
                                materialAttribute.Albedo = activeMaterial->Albedo;
                                materialAttribute.Metallic = activeMaterial->Metallic;
                                materialAttribute.Roughness = activeMaterial->Roughness;
                                materialAttribute.AlphaCut = activeMaterial->AlphaCut ? 1 : 0;
                                materialAttribute.CastShadows = activeMaterial->CastShadows ? 1 : 0;
                            }
                            materialAttribute.DiffuseTextureID = -1;
                            materialAttribute.NormalTextureID = -1;
                            materialAttribute.ORMTextureID = -1;

                            if(activeMaterial)
                            {
                                if(activeMaterial->HasDiffuseTexture())
                                    materialAttribute.DiffuseTextureID = activeMaterial->GetDiffuseTexture()->GetIndex(SRV_CBV);
                                if(activeMaterial->HasNormalTexture())
                                    materialAttribute.NormalTextureID = activeMaterial->GetNormalTexture()->GetIndex(SRV_CBV);
                                if(activeMaterial->HasORMTexture())
                                    materialAttribute.ORMTextureID = activeMaterial->GetORMTexture()->GetIndex(SRV_CBV);
                            }

                            MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                            DrawCommandsBuffer.UpdateData(&skeletalMeshComponent.DrawCommand, sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                            auto& transform = entity.get<Transform>();

                            // Clean up previous bone matrices buffer if this entity was already registered
                            // (use the map as source of truth, not the component field, to avoid stale pointer issues)
                            SkeletalSkinningEntry* existingEntry = SkeletalSkinningData.Find(entity.id());
                            if(existingEntry && existingEntry->BoneMatricesBuffer)
                            {
                                Renderer::Destroy(existingEntry->BoneMatricesBuffer);
                                delete existingEntry->BoneMatricesBuffer;
                                existingEntry->BoneMatricesBuffer = nullptr;
                                skeletalMeshComponent.BoneMatricesBuffer = nullptr;
                            }

                            int boneCount = skeletalMesh->BoneCount > 0 ? skeletalMesh->BoneCount : 1;
                            skeletalMeshComponent.BoneCount = boneCount;

                            WArray<Matrix4> identityMatrices;
                            identityMatrices.Resize(boneCount, Matrix4(1.0f));
                            skeletalMeshComponent.BoneMatricesBuffer = Renderer::CreateBuffer(
                                "BoneMatricesBuffer",
                                StorageBuffer,
                                boneCount * sizeof(Matrix4),
                                sizeof(Matrix4),
                                identityMatrices.GetData()
                            );

                            // Register skinning data for the per-frame skinning compute dispatch
                            SkeletalSkinningEntry& entry = SkeletalSkinningData[entity.id()];
                            entry.BindPoseVertexSRV  = skeletalMesh->VertexBuffer->GetIndex(SRV_CBV);
                            entry.VertexBonesSRV     = skeletalMesh->VertexBonesBuffer->GetIndex(SRV_CBV);
                            entry.BoneMatricesBuffer = skeletalMeshComponent.BoneMatricesBuffer;
                            entry.VertexOffset       = skeletalMeshComponent.DrawCommand.BaseVertexLocation;
                            entry.VertexCount        = vertexCount;
                            entry.DrawCommand        = skeletalMeshComponent.DrawCommand;
                            entry.GlobalDrawId       = globalDrawId;

                            Renderer::Wait();
                            Renderer::RenderData.TLAS.SetData(globalDrawId, skeletalMesh->Name, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, skeletalMeshComponent.DrawCommand, vertexCount, transform);
                        }
                    }
                }
            });

            ECS::World.observer<SkeletalMeshComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, SkeletalMeshComponent& skeletalMeshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int bfcDrawId;

                    if(IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId))
                    {
                        IndirectIndexedCommand& command = BFCIndirectCommands[bfcDrawId];

                        if(skeletalMeshComponent.MeshRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(skeletalMeshComponent.MeshRef.Mesh->VertexData.GetSize(), command.Command.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(skeletalMeshComponent.MeshRef.Mesh->IndexData.GetSize(), command.Command.StartIndexLocation * sizeof(uint));

                            VerticesCount -= skeletalMeshComponent.MeshRef.Mesh->VertexData.Num();
                            IndicesCount  -= skeletalMeshComponent.MeshRef.Mesh->IndexData.Num();
                        }

                        BFCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), bfcDrawId * sizeof(IndirectIndexedCommand));

                        command.DrawId  = -1;
                        command.Command = { 0, 0, 0, 0, 0 };

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);

                        if(entity.has<Transform>())
                        {
                            WorldTransformsBuffer.RemoveData(sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        }

                        if(skeletalMeshComponent.BoneMatricesBuffer)
                        {
                            Renderer::Destroy(skeletalMeshComponent.BoneMatricesBuffer);
                            delete skeletalMeshComponent.BoneMatricesBuffer;
                            skeletalMeshComponent.BoneMatricesBuffer = nullptr;
                        }

                        SkeletalSkinningData.Remove(entity.id());

                        IdManager::RemoveId(entity, BackFaceCullingDrawIdType);
                    }

                    IdManager::RemoveId(entity, GlobalDrawIdType);
                }
            });

            ECS::World.observer<Sprite, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Sprite& sprite, Transform& transform)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                auto ncDrawId = IdManager::AddId(entity, NoCullingDrawIdType);
                
                if(ncDrawId >= NCIndirectCommands.Num())
                {
                    NCIndirectCommands.Add(IndirectIndexedCommand());
                    NCIndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectIndexedCommand), ncDrawId * sizeof(IndirectIndexedCommand));
                }
                
                WorldTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.UpdateOrAdd(nullptr, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.UpdateOrAdd(nullptr, sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData(globalDrawId);
            });

            ECS::World.observer<Sprite>().event(flecs::OnSet).each([&](flecs::entity entity, Sprite& sprite)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int ncDrawId;
                    
                    if(IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                    {
                        bool textureReferenceIsEmpty = sprite.TextureRef.Reference.empty() || sprite.TextureRef.Reference == "Empty";
                        
                        if(textureReferenceIsEmpty && !sprite.TextureRef.IsValid())
                        {
                            return;
                        }

                        if(!textureReferenceIsEmpty && !sprite.TextureRef.IsValid())
                        {
                            sprite.TextureRef.LoadAsset();
                        }
                        
                        if(sprite.TextureRef.IsValid())
                        {
                            sprite.DrawCommand = {
                                (uint)SpriteIndices.Num(),
                                1,
                                (uint)IndicesCount,
                                (int)VerticesCount,
                                0
                            };
                            auto& command = NCIndirectCommands[ncDrawId];
                            command.DrawId = globalDrawId;
                            command.Command = sprite.DrawCommand;

                            NCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * ncDrawId);

                            Renderer::RenderData.VertexBuffer.AddData(SpriteVertices.GetData(), SpriteVertices.GetSize());
                            Renderer::RenderData.IndexBuffer.AddData(SpriteIndices.GetData(), SpriteIndices.GetSize());

                            VerticesCount += SpriteVertices.Num();
                            IndicesCount += SpriteIndices.Num();
                            
                            auto& materialAttribute = MaterialAttributes[globalDrawId];

                            materialAttribute.Albedo = sprite.Color;
                            materialAttribute.AlphaCut = sprite.AlphaCut ? 1 : 0;
                            materialAttribute.DiffuseTextureID = sprite.TextureRef.Texture->GetIndex(SRV_CBV);

                            MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), sizeof(MaterialShaderAttribute) * globalDrawId);
                            DrawCommandsBuffer.UpdateData(&sprite.DrawCommand, sizeof(DrawIndexedCommand), sizeof(DrawIndexedCommand) * globalDrawId);
                        }

                        auto transform = entity.get<Transform>();

                        WString spriteName = "Sprite_";
                        spriteName += sprite.TextureRef.Texture->GetName(); 

                        Renderer::RenderData.TLAS.SetData(globalDrawId, spriteName, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, sprite.DrawCommand, SpriteVertices.Num(), transform);
                    }
                }
            });

            ECS::World.observer<Sprite>().event(flecs::OnRemove).each([&](flecs::entity entity, Sprite& sprite)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int ncDrawId;
                    if(IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                    {
                        IndirectIndexedCommand& command = NCIndirectCommands[ncDrawId];

                        if(sprite.TextureRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(SpriteVertices.GetSize(), command.Command.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(SpriteIndices.GetSize(), command.Command.StartIndexLocation * sizeof(uint));

                            VerticesCount -= SpriteVertices.Num();
                            IndicesCount -= SpriteIndices.Num();
                        }
                        
                        NCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), ncDrawId * sizeof(IndirectIndexedCommand));
                        
                        command.DrawId = -1;
                        command.Command = { 0, 0, 0, 0, 0 };

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);
                        
                        if(entity.has<Transform>())
                        {
                            WorldTransformsBuffer.RemoveData(sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        }

                        IdManager::RemoveId(entity, NoCullingDrawIdType);
                    }

                    IdManager::RemoveId(entity, GlobalDrawIdType);
                }
            });
            
            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    WorldTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                    
                    Renderer::RenderData.TLAS.UpdateTransform(globalDrawId, transform);
                }
                
                if(entity.has<Light>())
                {
                    int lightId;

                    if(IdManager::GetId(entity, LightIdType, lightId))
                    {
                        LightTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    }
                }
            });
            
            ECS::World.observer<Light, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Light& light, Transform& transform)
            {
                auto lightId = IdManager::AddId(entity, LightIdType);

                LightsBuffer.UpdateOrAdd(&light, sizeof(Light), sizeof(Light) * lightId);
                LightTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), sizeof(Matrix4) * lightId);
                LightsIndices.Add(lightId);
                LightsIndicesBuffer.UpdateOrAdd(LightsIndices.GetData(), LightsIndices.GetSize(), 0);
                
                RayTracingSceneData.NumLights++;
            });

            ECS::World.system<Transform>("HybridRenderingTransformSyncSystem").kind<ECS::OnDraw>().each([&](flecs::entity entity, Transform& transform)
            {
                int globalDrawId;
                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    WorldTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                    Renderer::RenderData.TLAS.UpdateTransform(globalDrawId, transform);
                }

                if(entity.has<Light>())
                {
                    int lightId;
                    if(IdManager::GetId(entity, LightIdType, lightId))
                    {
                        LightTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    }
                }
            });
            
            ECS::World.observer<Light>().event(flecs::OnSet).each([&](flecs::entity entity, Light& light)
            {
                int lightId;

                if(IdManager::GetId(entity, LightIdType, lightId))
                {
                    LightsBuffer.UpdateData(&light, sizeof(Light), sizeof(Light) * lightId);
                }
            });
            
            ECS::World.observer<Light>().event(flecs::OnRemove).each([&](flecs::entity entity, Light& light)
            {
                int lightId;

                if(IdManager::GetId(entity, LightIdType, lightId))
                {
                    LightTransformsBuffer.RemoveData(sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    LightsBuffer.RemoveData(sizeof(Light), sizeof(Light) * lightId);
                    LightsIndices.Remove(lightId);
                    LightsIndicesBuffer.UpdateData(LightsIndices.GetData(), LightsIndices.GetSize(), 0);
                    RayTracingSceneData.NumLights--;

                    IdManager::RemoveId(entity, LightIdType);
                }
            });

            ECS::World.observer<Sky>().event(flecs::OnSet).yield_existing().each([&](Sky& skybox)
            {
                SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
            });

            ECS::World.system("SkyColorClearingSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                viewport->GetGBuffer()->Clear({SkyColor});
            });
            
            ECS::World.system<Sky>("SkyRenderingSystem").kind<ECS::OnDraw>().each([&](Sky& skybox)
            {
                if(!Renderer::RenderData.FeatureToggles.EnableSkyPass)
                {
                    return;
                }

                auto q = ECS::World.query<Light, Transform>();

                bool hasSun = false;
                Vector3 sunDirection = Vector3(0, -1, 0);
                
                q.each([&hasSun, &sunDirection](ECS::Entity e, Light& light, Transform& transform)
                {
                    if (light.Type == LightType::Directional)
                    {
                        hasSun = true;
                        
                        sunDirection = -transform.GetForwardVector();

                        return;
                    }
                });

                if (!hasSun)
                {
                    return;
                }

                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto skyColor = viewport->GetGBufferRenderTarget(SkyColor);
                    if(!linkedCamera.is_alive() || !linkedCamera.has<Camera>() || !linkedCamera.has<Transform>())
                    {
                        return;
                    }
                    auto& cameraComponent = linkedCamera.get<Camera>();
                    auto& transformComponent = linkedCamera.get<Transform>();

                    SkyPassSceneData.InverseProjection = inverse(cameraComponent.ProjectionMatrix);
                    SkyPassSceneData.InverseView = transformComponent.RenderMatrix;
                    SkyPassSceneData.ViewProjection = cameraComponent.ProjectionMatrix * inverse(transformComponent.RenderMatrix);
                    SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                    SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                    SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
                    SkyPassSceneData.SunDirection = Vector4(sunDirection, 1.0f);
                    SkyPassSceneData.CameraPosition = Vector4(transformComponent.RenderPosition, 1.0f);
                    
                    Renderer::UploadBuffer(SceneDataBuffer, &SkyPassSceneData, sizeof(SkySceneData));
                    Renderer::ResourceBarrier(skyColor, ALL_SHADER_RESOURCE, RENDER_TARGET);
                    Renderer::BindRenderTargets(skyColor);
                    Renderer::BindDepthStencil(nullptr);
                    Renderer::SetPipeline(SkyPipeline);
                    Renderer::PushConstants(&SkyPassRootConstants, sizeof(SkyRootConstants));
                    Renderer::Draw(&FullscreenQuad);
                    Renderer::ResourceBarrier(skyColor, RENDER_TARGET, ALL_SHADER_RESOURCE);
                }
            });
            
            ECS::World.system("SkeletalMeshSkinningSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                if(SkeletalSkinningData.IsEmpty())
                    return;

                Renderer::ResourceBarrier(Renderer::RenderData.VertexBuffer.GetBuffer(), UNORDERED_ACCESS);

                for(uint skinIdx = 0; skinIdx < SkeletalSkinningData.Num(); ++skinIdx)
                {
                    auto& entry = SkeletalSkinningData.At(skinIdx).value;

                    SkinningConstants.BindPoseVertexBuffer = entry.BindPoseVertexSRV;
                    SkinningConstants.SkinnedVertexBuffer  = Renderer::RenderData.VertexBuffer.GetIndex(UAV);
                    SkinningConstants.VertexBonesBuffer    = entry.VertexBonesSRV;
                    SkinningConstants.BoneMatricesBuffer   = entry.BoneMatricesBuffer->GetIndex(SRV_CBV);
                    SkinningConstants.VertexOffset         = (uint)entry.VertexOffset;
                    SkinningConstants.VertexCount          = entry.VertexCount;

                    Renderer::SetPipeline(SkinningPipeline);
                    Renderer::PushConstants(&SkinningConstants, sizeof(SkinningRootConstants));

                    uint groupCount = (entry.VertexCount + 63) / 64;
                    Renderer::Compute(Point3(groupCount, 1, 1));
                }

                Renderer::UAVBarrier(Renderer::RenderData.VertexBuffer.GetBuffer());
                Renderer::ResourceBarrier(Renderer::RenderData.VertexBuffer.GetBuffer(), (ResourceStates)(VERTEX_AND_CONSTANT_BUFFER | NON_PIXEL_SHADER_RESOURCE));

                for(uint skinIdx = 0; skinIdx < SkeletalSkinningData.Num(); ++skinIdx)
                {
                    auto& entry = SkeletalSkinningData.At(skinIdx).value;

                    Renderer::RenderData.TLAS.UpdateGeometry(
                        entry.GlobalDrawId,
                        Renderer::RenderData.VertexBuffer.GetBuffer(),
                        Renderer::RenderData.IndexBuffer.GetBuffer(),
                        entry.DrawCommand,
                        entry.VertexCount
                    );
                }
            });

            ECS::World.system("GBufferClearSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                SViewport* viewport = Renderer::GetCurrentViewport();
                viewport->GetGBuffer()->Clear({WorldPosition, Normal, Color, ORM, MeshID, Depth});
            });
            
            ECS::World.system("GBufferSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                // Publish shared scene buffers for offline training systems.
                Renderer::RenderData.SharedDrawCommandsBuffer = DrawCommandsBuffer.GetBuffer();
                Renderer::RenderData.SharedMaterialAttributesBuffer = MaterialAttributesBuffer.GetBuffer();
                Renderer::RenderData.SharedWorldTransformsBuffer = WorldTransformsBuffer.GetBuffer();
                Renderer::RenderData.SharedLightsBuffer = LightsBuffer.GetBuffer();
                Renderer::RenderData.SharedLightTransformsBuffer = LightTransformsBuffer.GetBuffer();
                Renderer::RenderData.SharedLightsIndicesBuffer = LightsIndicesBuffer.GetBuffer();
                Renderer::RenderData.SharedDrawCommandsCount = static_cast<uint>(BFCIndirectCommands.Num() + NCIndirectCommands.Num());
                Renderer::RenderData.SharedNumLights = static_cast<uint>(LightsIndices.Num());

                if(!Renderer::RenderData.FeatureToggles.EnableGBufferPass)
                {
                    return;
                }

                if(BFCIndirectCommands.Num() <= 0 && NCIndirectCommands.Num() <= 0)
                {
                    return;
                }
                
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    if(!linkedCamera.is_alive() || !linkedCamera.has<Camera>() || !linkedCamera.has<Transform>())
                    {
                        return;
                    }
                    auto& cameraComponent = linkedCamera.get<Camera>();
                    auto& transformComponent = linkedCamera.get<Transform>();
                    
                    GBufferSceneData.ProjectionMatrix = cameraComponent.ProjectionMatrix;
                    GBufferSceneData.ViewMatrix = inverse(transformComponent.RenderMatrix);
                    GBufferSceneData.WorldMatrix = transformComponent.RenderMatrix;
                    GBufferSceneData.InverseProjectionMatrix = inverse(cameraComponent.ProjectionMatrix);
                    
                    Renderer::UploadBuffer(GBufferSceneDataBuffer, &GBufferSceneData, sizeof(SGBufferSceneData));

                    auto gbuffer = viewport->GetGBuffer();

                    GBufferRootConstants.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_CBV);
                    GBufferRootConstants.MaterialAttributes = MaterialAttributesBuffer.GetIndex(SRV_CBV);
                    GBufferRootConstants.SceneDataBuffer = GBufferSceneDataBuffer->GetIndex(SRV_CBV);

                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, ALL_SHADER_RESOURCE, RENDER_TARGET);
                    gbuffer->Barrier(Depth, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    
                    // Back face culling pass
                    if(BFCIndirectCommands.Num() > 0)
                    {
                        Renderer::SetPipeline(BFCGBufferPipeline);
                        Renderer::PushConstants(&GBufferRootConstants, sizeof(GBufferRootConstants));
                        Renderer::BindRenderTargets({ gbuffer->GetRenderTarget(WorldPosition), gbuffer->GetRenderTarget(Normal), gbuffer->GetRenderTarget(Color), gbuffer->GetRenderTarget(ORM), gbuffer->GetRenderTarget(MeshID) });
                        Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                        Renderer::SetVertexBuffers(Renderer::RenderData.VertexBuffer.GetBuffer(), 1);
                        Renderer::SetIndexBuffer(Renderer::RenderData.IndexBuffer.GetBuffer());
                        Renderer::DrawIndirect(BFCIndirectCommands.Num(), BFCIndirectBuffer);
                    }
                    
                    // No culling pass
                    if(NCIndirectCommands.Num() > 0)
                    {
                        Renderer::SetPipeline(NCGBufferPipeline);
                        Renderer::PushConstants(&GBufferRootConstants, sizeof(GBufferRootConstants));
                        Renderer::BindRenderTargets({ gbuffer->GetRenderTarget(WorldPosition), gbuffer->GetRenderTarget(Normal), gbuffer->GetRenderTarget(Color), gbuffer->GetRenderTarget(ORM), gbuffer->GetRenderTarget(MeshID) });
                        Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                        Renderer::SetVertexBuffers(Renderer::RenderData.VertexBuffer.GetBuffer(), 1);
                        Renderer::SetIndexBuffer(Renderer::RenderData.IndexBuffer.GetBuffer());
                        Renderer::DrawIndirect(NCIndirectCommands.Num(), NCIndirectBuffer);
                    }
                    
                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, RENDER_TARGET, ALL_SHADER_RESOURCE);
                    gbuffer->Barrier(Depth, DEPTH_WRITE, ALL_SHADER_RESOURCE);
                }
            });

            ECS::World.system("RayTracingSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto gbuffer = viewport->GetGBuffer();
                    auto radianceRT = gbuffer->GetRenderTarget(Radiance);

                    if(!Renderer::RenderData.FeatureToggles.EnableRayTracingPass)
                    {
                        gbuffer->Clear({ Radiance, Reflection });
                        return;
                    }

                    if(!linkedCamera.is_alive() || !linkedCamera.has<Camera>() || !linkedCamera.has<Transform>())
                    {
                        return;
                    }
                    auto& cameraComponent = linkedCamera.get<Camera>();
                    auto& transformComponent = linkedCamera.get<Transform>();

                    RayTracingSceneData.CameraPosition = transformComponent.RenderPosition;
                    RayTracingSceneData.InvViewMatrix = transformComponent.RenderMatrix;
                    RayTracingSceneData.InvProjectionMatrix = inverse(cameraComponent.ProjectionMatrix);
                    Renderer::UploadBuffer(RayTracingSceneDataBuffer, &RayTracingSceneData, sizeof(SRayTracingSceneData));

                    auto& toggles = Renderer::RenderData.FeatureToggles;
                    bool cameraChanged =
                        !PathTracingHistoryValid ||
                        memcmp(&LastPathTracingInvView, &RayTracingSceneData.InvViewMatrix, sizeof(Matrix4)) != 0 ||
                        memcmp(&LastPathTracingInvProjection, &RayTracingSceneData.InvProjectionMatrix, sizeof(Matrix4)) != 0 ||
                        memcmp(&LastPathTracingCameraPosition, &RayTracingSceneData.CameraPosition, sizeof(Vector3)) != 0;

                    if(toggles.EnablePathTracing)
                    {
                        if(cameraChanged || !toggles.EnablePathTracingAccumulation)
                        {
                            PathTracingFrameIndex = 0;
                        }
                        else
                        {
                            PathTracingFrameIndex++;
                        }
                    }
                    else
                    {
                        PathTracingFrameIndex = 0;
                    }

                    PathTracingHistoryValid = true;
                    LastPathTracingInvView = RayTracingSceneData.InvViewMatrix;
                    LastPathTracingInvProjection = RayTracingSceneData.InvProjectionMatrix;
                    LastPathTracingCameraPosition = RayTracingSceneData.CameraPosition;

                    RayTracingRootConstants.WorldPositionRT = gbuffer->GetRenderTarget(WorldPosition)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.NormalRT = gbuffer->GetRenderTarget(Normal)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.ColorRT = gbuffer->GetRenderTarget(Color)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.ORMRT = gbuffer->GetRenderTarget(ORM)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.RadianceRT = radianceRT->GetIndex(SRV_CBV);
                    RayTracingRootConstants.PathTracingRT = gbuffer->GetRenderTarget(PathTracing)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.ReflectionRT = gbuffer->GetRenderTarget(Reflection)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.LightsBuffer = LightsBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.LightTransformsBuffer = LightTransformsBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.LightsIndicesBuffer = LightsIndicesBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.SceneDataBuffer = RayTracingSceneDataBuffer->GetIndex(SRV_CBV);
                    RayTracingRootConstants.TLAS = Renderer::RenderData.TLAS.GetIndex(SRV_CBV);
                    RayTracingRootConstants.VertexBuffer = Renderer::RenderData.VertexBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.IndexBuffer = Renderer::RenderData.IndexBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.DrawCommandsBuffer = DrawCommandsBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.MaterialBuffer = MaterialAttributesBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.EnableReflections = toggles.EnableReflections ? 1 : 0;
                    RayTracingRootConstants.EnableDirectLighting = toggles.EnableDirectLighting ? 1 : 0;
                    RayTracingRootConstants.EnableSpecular = toggles.EnableSpecular ? 1 : 0;
                    RayTracingRootConstants.EnableMetallic = toggles.EnableMetallic ? 1 : 0;
                    RayTracingRootConstants.EnablePathTracing = toggles.EnablePathTracing ? 1 : 0;
                    RayTracingRootConstants.PathTracingMaxBounces = toggles.PathTracingMaxBounces > 4 ? 4 : toggles.PathTracingMaxBounces;
                    RayTracingRootConstants.PathTracingSamplesPerPixel = toggles.PathTracingSamplesPerPixel;
                    RayTracingRootConstants.PathTracingFrameIndex = PathTracingFrameIndex;
                    RayTracingRootConstants.EnablePathTracingAccumulation = toggles.EnablePathTracingAccumulation ? 1 : 0;

                    //dispatching
                    viewport->GetGBuffer()->Barriers({Radiance, PathTracing, Reflection}, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Pipeline* activeRTPipeline = toggles.EnablePathTracing ? PathTracingPipeline : RTPipeline;
                    Renderer::SetPipeline(activeRTPipeline);
                    Renderer::PushConstants(&RayTracingRootConstants, sizeof(RayTracingRootConstants));
                    Renderer::TraceRays(activeRTPipeline, Point3(radianceRT->GetWidth(), radianceRT->GetHeight(), 1));
                    viewport->GetGBuffer()->Barriers({Radiance, PathTracing, Reflection}, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });

            ECS::World.system("DeferredRenderingSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto gbuffer = viewport->GetGBuffer();
                    
                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    gbuffer->Clear({Deferred, NIVIrradiance});

                    if(!Renderer::RenderData.FeatureToggles.EnableDeferredPass)
                    {
                        return;
                    }

                    Vector2 resolution = Vector2(deferredRT->GetWidth(), deferredRT->GetHeight());
                    const uint width = (uint)resolution.x;
                    const uint height = (uint)resolution.y;
                    const uint pixelCount = width * height;
                    const uint selectedOutputTarget = viewport->Type == EditorViewport ? Renderer::RenderData.EditorViewportOutputTarget : Renderer::RenderData.GameViewportOutputTarget;

                    bool hasNIVPrediction = false;
                    const bool needNIV = Renderer::RenderData.FeatureToggles.EnableNIVInference || selectedOutputTarget == 8;
                    auto* nivCoach = reinterpret_cast<Coach::TinyCuda::NIVCoach*>(Renderer::RenderData.NIVRuntimeCoach);
                    Renderer::RenderData.NIVLastInferenceAttempted = needNIV;
                    Renderer::RenderData.NIVLastInferenceSucceeded = false;
                    Renderer::RenderData.NIVLastValidPixelCount = 0;
                    Renderer::RenderData.NIVLastMeanLuminance = 0.0f;
                    Renderer::RenderData.NIVLastMaxChannel = 0.0f;

                    if(needNIV && nivCoach && pixelCount > 0)
                    {
                        try
                        {
                            if(pixelCount > NIVBufferCapacityPixels)
                            {
                                if(NIVWorldPositionBuffer)
                                {
                                    Renderer::Destroy(NIVWorldPositionBuffer);
                                    delete NIVWorldPositionBuffer;
                                    NIVWorldPositionBuffer = nullptr;
                                }

                                if(NIVWorldNormalValidBuffer)
                                {
                                    Renderer::Destroy(NIVWorldNormalValidBuffer);
                                    delete NIVWorldNormalValidBuffer;
                                    NIVWorldNormalValidBuffer = nullptr;
                                }

                                if(NIVPredictedOutputBuffer)
                                {
                                    Renderer::Destroy(NIVPredictedOutputBuffer);
                                    delete NIVPredictedOutputBuffer;
                                    NIVPredictedOutputBuffer = nullptr;
                                }

                                if(NIVPredictedHistoryBuffer)
                                {
                                    Renderer::Destroy(NIVPredictedHistoryBuffer);
                                    delete NIVPredictedHistoryBuffer;
                                    NIVPredictedHistoryBuffer = nullptr;
                                }

                                NIVWorldPositionBuffer = Renderer::CreateBuffer(
                                    "NIVWorldPositionBuffer",
                                    StorageBuffer,
                                    pixelCount * sizeof(float) * 4,
                                    sizeof(float) * 4
                                );
                                NIVWorldNormalValidBuffer = Renderer::CreateBuffer(
                                    "NIVWorldNormalValidBuffer",
                                    StorageBuffer,
                                    pixelCount * sizeof(float) * 4,
                                    sizeof(float) * 4
                                );
                                NIVPredictedOutputBuffer = Renderer::CreateBuffer(
                                    "NIVPredictedOutputBuffer",
                                    StorageBuffer,
                                    pixelCount * sizeof(float) * 4,
                                    sizeof(float) * 4
                                );
                                NIVPredictedHistoryBuffer = Renderer::CreateBuffer(
                                    "NIVPredictedHistoryBuffer",
                                    StorageBuffer,
                                    pixelCount * sizeof(float) * 4,
                                    sizeof(float) * 4
                                );
                                NIVBufferCapacityPixels = pixelCount;
                            }

                            NIVPackRootConstants.WorldPositionRT = gbuffer->GetRenderTarget(WorldPosition)->GetIndex(SRV_CBV);
                            NIVPackRootConstants.WorldNormalRT = gbuffer->GetRenderTarget(Normal)->GetIndex(SRV_CBV);
                            NIVPackRootConstants.MeshIDRT = gbuffer->GetRenderTarget(MeshID)->GetIndex(SRV_CBV);
                            NIVPackRootConstants.OutWorldPositionBuffer = NIVWorldPositionBuffer->GetIndex(UAV);
                            NIVPackRootConstants.OutWorldNormalValidBuffer = NIVWorldNormalValidBuffer->GetIndex(UAV);
                            NIVPackRootConstants.Width = width;
                            NIVPackRootConstants.Height = height;

                            Renderer::ResourceBarrier(NIVWorldPositionBuffer, UNORDERED_ACCESS);
                            Renderer::ResourceBarrier(NIVWorldNormalValidBuffer, UNORDERED_ACCESS);
                            Renderer::SetPipeline(NIVPackPipeline);
                            Renderer::PushConstants(&NIVPackRootConstants, sizeof(NIVPackRootConstants));
                            Point3 nivPackThreads = Renderer::GetNumThreadsPerGroup(NIVPackComputeShader);
                            Point3 nivPackGroupCount = Point3((width + nivPackThreads.x - 1) / nivPackThreads.x, (height + nivPackThreads.y - 1) / nivPackThreads.y, 1);
                            Renderer::Compute(nivPackGroupCount);
                            Renderer::ResourceBarrier(NIVWorldPositionBuffer, ALL_SHADER_RESOURCE);
                            Renderer::ResourceBarrier(NIVWorldNormalValidBuffer, ALL_SHADER_RESOURCE);

                            Renderer::Flush();

                            uint validPixels = 0;
                            float meanLuminance = 0.0f;
                            float maxChannel = 0.0f;
                            const bool interopSucceeded = nivCoach->InferFromSharedBuffers(
                                Renderer::GetSharedHandle(NIVWorldPositionBuffer),
                                pixelCount * sizeof(float) * 4,
                                Renderer::GetSharedHandle(NIVWorldNormalValidBuffer),
                                pixelCount * sizeof(float) * 4,
                                Renderer::GetSharedHandle(NIVPredictedOutputBuffer),
                                pixelCount * sizeof(float) * 4,
                                Renderer::GetSharedHandle(NIVPredictedHistoryBuffer),
                                pixelCount * sizeof(float) * 4,
                                pixelCount,
                                Renderer::RenderData.EnableNIVTemporalSmoothing,
                                Renderer::RenderData.NIVTemporalHistoryWeight,
                                &validPixels,
                                &meanLuminance,
                                &maxChannel
                            );

                            if(interopSucceeded)
                            {
                                NIVPredictedOutputBuffer->SetCurrentState(ALL_SHADER_RESOURCE);
                                if (NIVPredictedHistoryBuffer)
                                {
                                    NIVPredictedHistoryBuffer->SetCurrentState(ALL_SHADER_RESOURCE);
                                }

                                Renderer::RenderData.NIVLastInferenceSucceeded = true;
                                Renderer::RenderData.NIVLastValidPixelCount = validPixels;
                                Renderer::RenderData.NIVLastMeanLuminance = meanLuminance;
                                Renderer::RenderData.NIVLastMaxChannel = maxChannel;
                                hasNIVPrediction = true;
                            }
                            else
                            {
                                std::vector<float> worldPositionRGBA(pixelCount * 4, 0.0f);
                                std::vector<float> worldNormalValidRGBA(pixelCount * 4, 0.0f);
                                std::vector<float> predictedRGBA(pixelCount * 4, 0.0f);
                                Renderer::DownloadBuffer(NIVWorldPositionBuffer, worldPositionRGBA.data(), worldPositionRGBA.size() * sizeof(float));
                                Renderer::DownloadBuffer(NIVWorldNormalValidBuffer, worldNormalValidRGBA.data(), worldNormalValidRGBA.size() * sizeof(float));
                                nivCoach->InferFromBuffers(worldPositionRGBA.data(), worldNormalValidRGBA.data(), pixelCount, predictedRGBA.data());
                                Renderer::UploadBuffer(NIVPredictedOutputBuffer, predictedRGBA.data(), (uint32_t)(predictedRGBA.size() * sizeof(float)), 0);
                                if(NIVPredictedOutputBuffer->GetCurrentState() != ALL_SHADER_RESOURCE)
                                {
                                    Renderer::ResourceBarrier(NIVPredictedOutputBuffer, ALL_SHADER_RESOURCE);
                                }

                                double lumAccum = 0.0;
                                for(uint i = 0; i < pixelCount; ++i)
                                {
                                    const float valid = worldNormalValidRGBA[i * 4 + 3];
                                    if(valid <= 0.5f)
                                    {
                                        continue;
                                    }
                                    validPixels++;
                                    const float r = predictedRGBA[i * 4 + 0];
                                    const float g = predictedRGBA[i * 4 + 1];
                                    const float b = predictedRGBA[i * 4 + 2];
                                    lumAccum += (double)r * 0.2126 + (double)g * 0.7152 + (double)b * 0.0722;
                                    maxChannel = std::max(maxChannel, std::max(r, std::max(g, b)));
                                }
                                Renderer::RenderData.NIVLastInferenceSucceeded = true;
                                Renderer::RenderData.NIVLastValidPixelCount = validPixels;
                                Renderer::RenderData.NIVLastMeanLuminance = validPixels > 0 ? (float)(lumAccum / (double)validPixels) : 0.0f;
                                Renderer::RenderData.NIVLastMaxChannel = maxChannel;
                                hasNIVPrediction = true;
                            }
                        }
                        catch(...)
                        {
                            hasNIVPrediction = false;
                        }
                    }

                    gbuffer->Barriers({Deferred, NIVIrradiance}, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    
                    DeferredRootConstants.MeshIDRT = gbuffer->GetRenderTarget(MeshID)->GetIndex(SRV_CBV);
                    DeferredRootConstants.RadianceRT = gbuffer->GetRenderTarget(Radiance)->GetIndex(SRV_CBV);
                    DeferredRootConstants.PathTracingRT = gbuffer->GetRenderTarget(PathTracing)->GetIndex(SRV_CBV);
                    DeferredRootConstants.ColorRT = gbuffer->GetRenderTarget(Color)->GetIndex(SRV_CBV);
                    DeferredRootConstants.DeferredRT = deferredRT->GetIndex(UAV);
                    DeferredRootConstants.NIVIrradianceRT = gbuffer->GetRenderTarget(NIVIrradiance)->GetIndex(UAV);
                    DeferredRootConstants.NIVPredictedBuffer = NIVPredictedOutputBuffer ? NIVPredictedOutputBuffer->GetIndex(SRV_CBV) : 0;
                    DeferredRootConstants.SkyColorRT = gbuffer->GetRenderTarget(SkyColor)->GetIndex(SRV_CBV);
                    DeferredRootConstants.EnableSky = Renderer::RenderData.FeatureToggles.EnableSkyPass ? 1 : 0;
                    DeferredRootConstants.EnablePathTracing = Renderer::RenderData.FeatureToggles.EnablePathTracing ? 1 : 0;
                    DeferredRootConstants.EnableNIVInference = Renderer::RenderData.FeatureToggles.EnableNIVInference ? 1 : 0;
                    DeferredRootConstants.HasNIVPrediction = hasNIVPrediction ? 1 : 0;
                    DeferredRootConstants.EnableNIVSpatialFilter = Renderer::RenderData.EnableNIVSpatialFilter ? 1 : 0;
                    DeferredRootConstants.NIVSpatialFilterStrength = std::clamp(Renderer::RenderData.NIVSpatialFilterStrength, 0.0f, 1.0f);
                    Renderer::SetPipeline(DeferredRenderingPipeline);
                    Renderer::PushConstants(&DeferredRootConstants, sizeof(DeferredRootConstants));
                    
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
                    GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    
                    Renderer::Compute(GroupCount);
                    viewport->GetGBuffer()->Barriers({Deferred, NIVIrradiance}, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}
