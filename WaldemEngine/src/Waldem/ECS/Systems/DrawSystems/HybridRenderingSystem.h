#pragma once
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Sky.h"
#include "Waldem/ECS/Components/Sprite.h"
#include "Waldem/Renderer/ResizableAccelerationStructure.h"
#include "Waldem/Renderer/ResizableBuffer.h"

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
    
    struct RayTracingSceneData
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
        uint ReflectionRT;
        uint LightsBuffer;
        uint LightTransformsBuffer;
        uint SceneDataBuffer; 
        uint TLAS;
        uint VertexBuffer;
        uint IndexBuffer;
        uint DrawCommandsBuffer;
        uint MaterialBuffer;
    };
    
    struct DeferredRootConstants
    {
        Point2 MousePos;
        uint AlbedoRT;
        uint MeshIDRT;
        uint RadianceRT;
        uint TargetRT;
        uint SkyColorRT;
        uint DepthRTID; 
        uint HoveredMeshes;
        uint SceneDataBuffer;
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
    
    class WALDEM_API HybridRenderingSystem : public ISystem
    {
        //Sky pass
        Pipeline* SkyPipeline = nullptr;
        PixelShader* SkyPixelShader = nullptr;
        Quad FullscreenQuad = {};
        SkyRootConstants SkyPassRootConstants;
        SkySceneData SkyPassSceneData;
        Buffer* SceneDataBuffer = nullptr;
        RenderTarget* SkyColorRT = nullptr;
        bool SceneDataDirty = true;
        
        //GBuffer pass
        Pipeline* BFCGBufferPipeline = nullptr; // Back face culling GBuffer pipeline
        Pipeline* NCGBufferPipeline = nullptr; // No culling GBuffer pipeline
        PixelShader* GBufferPixelShader = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        ResizableBuffer DrawCommandsBuffer;
        ResizableBuffer BFCIndirectBuffer; //Back face culling indirect buffer
        WArray<IndirectCommand> BFCIndirectCommands;
        ResizableBuffer NCIndirectBuffer; //No culling indirect buffer
        WArray<IndirectCommand> NCIndirectCommands;
        ResizableBuffer WorldTransformsBuffer;
        ResizableBuffer MaterialAttributesBuffer;
        Buffer* GBufferSceneDataBuffer = nullptr;
        GBufferRootConstants GBufferRootConstants;
        bool CameraIsDirty = true;
        SGBufferSceneData GBufferSceneData;
        WArray<MaterialShaderAttribute> MaterialAttributes;
        size_t VerticesCount = 0;
        size_t IndicesCount = 0;

        //RayTracing
        RenderTarget* RadianceRT = nullptr;
        RenderTarget* ReflectionRT = nullptr;
        Pipeline* RTPipeline = nullptr;
        RayTracingShader* RTShader = nullptr;
        WArray<AccelerationStructure*> BLAS;
        WMap<CMesh*, AccelerationStructure*> BLASToUpdate;
        RayTracingSceneData RayTracingSceneData;
        ResizableBuffer LightsBuffer;
        ResizableBuffer LightTransformsBuffer;
        Buffer* RayTracingSceneDataBuffer = nullptr;
        RayTracingRootConstants RayTracingRootConstants;
        WArray<Matrix4> LightTransforms;

        //Deferred
        RenderTarget* TargetRT = nullptr;
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        Point3 GroupCount;
        DeferredRootConstants DeferredRootConstants;
        Buffer* HoveredMeshesBuffer = nullptr;

        //Sprite rendering
        WArray<Vertex> SpriteVertices;
        WArray<uint32> SpriteIndices;
        Buffer* SpriteVertexBuffer;
        Buffer* SpriteIndexBuffer;
        
    public:
        HybridRenderingSystem()
        {
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
        
        void Initialize(InputManager* inputManager) override
        {
            //Sky
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            SceneDataBuffer = Renderer::CreateBuffer("SkySceneDataBuffer", BufferType::StorageBuffer, sizeof(SkyPassSceneData), sizeof(SkyPassSceneData));
            SkyPassRootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_CBV);
            
            TargetRT = Renderer::GetRenderTarget("TargetRT");
            SkyColorRT = Renderer::CreateRenderTarget("SkyColorRT", TargetRT->GetWidth(), TargetRT->GetHeight(), TargetRT->GetFormat());
            
            SkyPixelShader = Renderer::LoadPixelShader("Sky");
            SkyPipeline = Renderer::CreateGraphicPipeline("SkyPipeline",
                                                            SkyPixelShader,
                                                            { TextureFormat::R8G8B8A8_UNORM },
                                                            TextureFormat::UNKNOWN,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            inputElementDescs);
            
            //GBuffer
            DrawCommandsBuffer = ResizableBuffer("DrawCommandsBuffer", BufferType::StorageBuffer, sizeof(DrawCommand), MAX_INDIRECT_COMMANDS);
            BFCIndirectBuffer = ResizableBuffer("BFCIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectCommand), MAX_INDIRECT_COMMANDS);
            NCIndirectBuffer = ResizableBuffer("NCIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectCommand), MAX_INDIRECT_COMMANDS);
            Renderer::RenderData.VertexBuffer = ResizableBuffer("VertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), 1000);
            Renderer::RenderData.IndexBuffer = ResizableBuffer("IndexBuffer", BufferType::IndexBuffer, sizeof(uint), 1000);
            WorldTransformsBuffer = ResizableBuffer("WorldTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), MAX_INDIRECT_COMMANDS);
            MaterialAttributesBuffer = ResizableBuffer("MaterialAttributesBuffer", BufferType::StorageBuffer, sizeof(MaterialShaderAttribute), MAX_INDIRECT_COMMANDS);
            GBufferSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(SGBufferSceneData), sizeof(SGBufferSceneData));
            
            WorldPositionRT = Renderer::GetRenderTarget("WorldPositionRT");
            NormalRT = Renderer::GetRenderTarget("NormalRT");
            ColorRT = Renderer::GetRenderTarget("ColorRT");
            ORMRT = Renderer::GetRenderTarget("ORMRT");
            MeshIDRT = Renderer::GetRenderTarget("MeshIDRT");
            DepthRT = Renderer::GetRenderTarget("DepthRT");
            
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            BFCGBufferPipeline = Renderer::CreateGraphicPipeline("BFCGBufferPipeline",
                                                            GBufferPixelShader,
                                                            { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), ColorRT->GetFormat(), ORMRT->GetFormat(), MeshIDRT->GetFormat() },
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
                                                            { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), ColorRT->GetFormat(), ORMRT->GetFormat(), MeshIDRT->GetFormat() },
                                                            TextureFormat::D32_FLOAT,
                                                            spriteRasterizer,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            //Raytracing
            RadianceRT = Renderer::GetRenderTarget("RadianceRT");
            ReflectionRT = Renderer::GetRenderTarget("ReflectionRT");
            RTShader = Renderer::LoadRayTracingShader("RayTracing/Radiance");
            RTPipeline = Renderer::CreateRayTracingPipeline("RayTracingPipeline", RTShader);
            LightsBuffer = ResizableBuffer("LightsBuffer", StorageBuffer, sizeof(LightData), 40);
            LightTransformsBuffer = ResizableBuffer("LightTransformsBuffer", StorageBuffer, sizeof(Matrix4), 40);
            RayTracingSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", StorageBuffer, sizeof(RayTracingSceneData), sizeof(RayTracingSceneData), &RayTracingSceneData);
            Renderer::RenderData.TLAS = ResizableAccelerationStructure("RayTracingTLAS", 50);
            
            //Deferred
            HoveredMeshesBuffer = Renderer::CreateBuffer("HoveredMeshes", StorageBuffer, sizeof(int), sizeof(int));
            DeferredRootConstants.HoveredMeshes = HoveredMeshesBuffer->GetIndex(UAV);
            DeferredRootConstants.DepthRTID = DepthRT->GetIndex(SRV_CBV);
            DeferredRootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_CBV);

            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingComputeShader);

            ECS::World.query<Camera, Transform>("SceneDataInitializationSystem").each([&](Camera& camera, Transform& transform)
            {
                GBufferSceneData.ViewMatrix = camera.ViewMatrix;
                GBufferSceneData.ProjectionMatrix = camera.ProjectionMatrix;
                GBufferSceneData.WorldMatrix = transform.Matrix;
                GBufferSceneData.InverseProjectionMatrix = glm::inverse(camera.ProjectionMatrix);
                
                CameraIsDirty = true;
            });
            
            ECS::World.observer<MeshComponent, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, MeshComponent& meshComponent, Transform& transform)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                auto bfcDrawId = IdManager::AddId(entity, BackFaceCullingDrawIdType);
                
                if(bfcDrawId >= BFCIndirectCommands.Num())
                {
                    BFCIndirectCommands.Add(IndirectCommand());
                    BFCIndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectCommand), bfcDrawId * sizeof(IndirectCommand));
                }                
                
                WorldTransformsBuffer.UpdateOrAdd(&transform.Matrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.AddData(nullptr, sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.AddData(nullptr, sizeof(DrawCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData();
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
                            meshComponent.DrawCommand = {
                                (uint)meshComponent.MeshRef.Mesh->IndexData.Num(),
                                1,
                                (uint)IndicesCount,
                                (int)VerticesCount,
                                0
                            };

                            uint vertexCount = meshComponent.MeshRef.Mesh->VertexData.Num();
                            
                            auto& command = BFCIndirectCommands[bfcDrawId];
                            command.DrawId = globalDrawId;
                            command.DrawIndexed = meshComponent.DrawCommand;

                            BFCIndirectBuffer.UpdateData(&command, sizeof(IndirectCommand), sizeof(IndirectCommand) * bfcDrawId);

                            Renderer::RenderData.VertexBuffer.AddData(meshComponent.MeshRef.Mesh->VertexData.GetData(), meshComponent.MeshRef.Mesh->VertexData.GetSize());
                            Renderer::RenderData.IndexBuffer.AddData(meshComponent.MeshRef.Mesh->IndexData.GetData(), meshComponent.MeshRef.Mesh->IndexData.GetSize());

                            VerticesCount += vertexCount;
                            IndicesCount += meshComponent.DrawCommand.IndexCountPerInstance;

                            auto& materialAttribute = MaterialAttributes[globalDrawId];

                            materialAttribute.Albedo = meshComponent.MeshRef.Mesh->MaterialRef.Mat->Albedo;
                            materialAttribute.Metallic = meshComponent.MeshRef.Mesh->MaterialRef.Mat->Metallic;
                            materialAttribute.Roughness = meshComponent.MeshRef.Mesh->MaterialRef.Mat->Roughness;
                            
                            if(meshComponent.MeshRef.Mesh->MaterialRef.Mat->HasDiffuseTexture())
                            {
                                materialAttribute.DiffuseTextureID = meshComponent.MeshRef.Mesh->MaterialRef.Mat->GetDiffuseTexture()->GetIndex(SRV_CBV);
                                
                                if(meshComponent.MeshRef.Mesh->MaterialRef.Mat->HasNormalTexture())
                                    materialAttribute.NormalTextureID = meshComponent.MeshRef.Mesh->MaterialRef.Mat->GetNormalTexture()->GetIndex(SRV_CBV);
                                if(meshComponent.MeshRef.Mesh->MaterialRef.Mat->HasORMTexture())
                                    materialAttribute.ORMTextureID = meshComponent.MeshRef.Mesh->MaterialRef.Mat->GetORMTexture()->GetIndex(SRV_CBV);
                            }

                            MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));

                            DrawCommandsBuffer.UpdateData(&meshComponent.DrawCommand, sizeof(DrawCommand), globalDrawId * sizeof(DrawCommand));
                            
                            auto transform = entity.get<Transform>();

                            Renderer::Wait();
                            Renderer::RenderData.TLAS.SetData(globalDrawId, meshComponent.MeshRef.Mesh->Name, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, meshComponent.DrawCommand, vertexCount, *transform);
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
                        IndirectCommand& command = BFCIndirectCommands[bfcDrawId];

                        if(meshComponent.MeshRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(meshComponent.MeshRef.Mesh->VertexData.GetSize(), command.DrawIndexed.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(meshComponent.MeshRef.Mesh->IndexData.GetSize(), command.DrawIndexed.StartIndexLocation * sizeof(uint));

                            VerticesCount -= meshComponent.MeshRef.Mesh->VertexData.Num();
                            IndicesCount -= meshComponent.MeshRef.Mesh->IndexData.Num();
                        }
                        
                        BFCIndirectBuffer.RemoveData(sizeof(IndirectCommand), bfcDrawId * sizeof(IndirectCommand));
                        
                        command.DrawId = -1;
                        command.DrawIndexed = { 0, 0, 0, 0, 0 };

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawCommand), globalDrawId * sizeof(DrawCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);
                        
                        auto transform = entity.get<Transform>();
                        
                        if(transform)
                        {
                            WorldTransformsBuffer.RemoveData(sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        }

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
                    NCIndirectCommands.Add(IndirectCommand());
                    NCIndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectCommand), ncDrawId * sizeof(IndirectCommand));
                }
                
                WorldTransformsBuffer.UpdateOrAdd(&transform.Matrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.UpdateOrAdd(nullptr, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.UpdateOrAdd(nullptr, sizeof(DrawCommand), globalDrawId * sizeof(DrawCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData();
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
                            command.DrawIndexed = sprite.DrawCommand;

                            NCIndirectBuffer.UpdateData(&command, sizeof(IndirectCommand), sizeof(IndirectCommand) * ncDrawId);

                            Renderer::RenderData.VertexBuffer.AddData(SpriteVertices.GetData(), SpriteVertices.GetSize());
                            Renderer::RenderData.IndexBuffer.AddData(SpriteIndices.GetData(), SpriteIndices.GetSize());

                            VerticesCount += SpriteVertices.Num();
                            IndicesCount += SpriteIndices.Num();
                            
                            auto& materialAttribute = MaterialAttributes[globalDrawId];

                            materialAttribute.Albedo = sprite.Color;
                            materialAttribute.DiffuseTextureID = sprite.TextureRef.Texture->GetIndex(SRV_CBV);

                            MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), sizeof(MaterialShaderAttribute) * globalDrawId);
                            DrawCommandsBuffer.UpdateData(&sprite.DrawCommand, sizeof(DrawCommand), sizeof(DrawCommand) * globalDrawId);
                        }

                        auto transform = entity.get<Transform>();

                        WString spriteName = "Sprite_";
                        spriteName += sprite.TextureRef.Texture->GetName(); 

                        Renderer::RenderData.TLAS.SetData(globalDrawId, spriteName, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, sprite.DrawCommand, SpriteVertices.Num(), *transform);
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
                        IndirectCommand& command = NCIndirectCommands[ncDrawId];

                        if(sprite.TextureRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(SpriteVertices.GetSize(), command.DrawIndexed.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(SpriteIndices.GetSize(), command.DrawIndexed.StartIndexLocation * sizeof(uint));

                            VerticesCount -= SpriteVertices.Num();
                            IndicesCount -= SpriteIndices.Num();
                        }
                        
                        NCIndirectBuffer.RemoveData(sizeof(IndirectCommand), ncDrawId * sizeof(IndirectCommand));
                        
                        command.DrawId = -1;
                        command.DrawIndexed = { 0, 0, 0, 0, 0 };

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawCommand), globalDrawId * sizeof(DrawCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);
                        
                        auto transform = entity.get<Transform>();
                        
                        if(transform)
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
                    WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                    
                    Renderer::RenderData.TLAS.UpdateTransform(globalDrawId, transform);
                }
                
                if(entity.has<Light>())
                {
                    int lightId;

                    if(IdManager::GetId(entity, LightIdType, lightId))
                    {
                        LightTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    }
                }
            });
            
            ECS::World.observer<Light, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Light& light, Transform& transform)
            {
                IdManager::AddId(entity, LightIdType);

                LightsBuffer.AddData(&light.Data, sizeof(LightData));
                LightTransformsBuffer.AddData(&transform.Matrix, sizeof(Matrix4));
                
                RayTracingSceneData.NumLights++;
            });
            
            ECS::World.observer<Light>().event(flecs::OnSet).each([&](flecs::entity entity, Light& light)
            {
                int lightId;

                if(IdManager::GetId(entity, LightIdType, lightId))
                {
                    LightsBuffer.UpdateData(&light.Data, sizeof(LightData), sizeof(LightData) * lightId);
                }
            });
            
            ECS::World.observer<Light>().event(flecs::OnRemove).each([&](flecs::entity entity, Light& light)
            {
                int lightId;

                if(IdManager::GetId(entity, LightIdType, lightId))
                {
                    LightTransformsBuffer.RemoveData(sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    LightsBuffer.RemoveData(sizeof(LightData), sizeof(LightData) * lightId);
                    RayTracingSceneData.NumLights--;

                    IdManager::RemoveId(entity, LightIdType);
                }
            });

            ECS::World.observer<Camera, Transform>("SceneDataUpdateSystem").event(flecs::OnSet).each([&](flecs::entity entity, Camera& camera, Transform& transform)
            {
                auto inverseProj = inverse(camera.ProjectionMatrix);
                auto world = transform.Matrix;
                GBufferSceneData.ViewMatrix = camera.ViewMatrix;
                GBufferSceneData.ProjectionMatrix = camera.ProjectionMatrix;
                GBufferSceneData.WorldMatrix = world;
                GBufferSceneData.InverseProjectionMatrix = inverseProj;
                RayTracingSceneData.InvViewMatrix = world;
                RayTracingSceneData.InvProjectionMatrix = inverseProj;
                RayTracingSceneData.CameraPosition = transform.Position;
                SkyPassSceneData.InverseProjection = inverseProj;
                SkyPassSceneData.InverseView = world;
                SkyPassSceneData.ViewProjection = camera.ViewProjectionMatrix;
                SkyPassSceneData.CameraPosition = Vector4(transform.Position, 1.0f);
                
                CameraIsDirty = true;
                SceneDataDirty = true;
            });

            ECS::World.observer<Sky>().event(flecs::OnAdd).yield_existing().each([&](flecs::entity entity, Sky& skybox)
            {
                SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
                SkyPassSceneData.SunDirection = Vector4(skybox.SunDirection, 1.0f);
                
                SceneDataDirty = true;
            });

            ECS::World.observer<Sky>().event(flecs::OnSet).yield_existing().each([&](Sky& skybox)
            {
                SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
                SkyPassSceneData.SunDirection = Vector4(skybox.SunDirection, 1.0f);
                
                SceneDataDirty = true;
            });

            ECS::World.system("SkyColorClearingSystem").kind(flecs::OnDraw).each([&]
            {
                Renderer::ResourceBarrier(SkyColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ClearRenderTarget(SkyColorRT);
                Renderer::ResourceBarrier(SkyColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            });
            
            ECS::World.system<Sky>("SkyRenderingSystem").kind(flecs::OnDraw).each([&](Sky& skybox)
            {
                if(SceneDataDirty)
                {
                    Renderer::UploadBuffer(SceneDataBuffer, &SkyPassSceneData, sizeof(SkySceneData));
                    SceneDataDirty = false;
                }
                
                Renderer::ResourceBarrier(SkyColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::BindRenderTargets(SkyColorRT);
                Renderer::BindDepthStencil(nullptr);
                Renderer::SetPipeline(SkyPipeline);
                Renderer::PushConstants(&SkyPassRootConstants, sizeof(SkyRootConstants));
                Renderer::Draw(&FullscreenQuad);
                Renderer::ResourceBarrier(SkyColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
            });
            
            ECS::World.system("GBufferClearSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                Renderer::ResourceBarrier(WorldPositionRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(NormalRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ORMRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(MeshIDRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);

                Renderer::ClearRenderTarget(WorldPositionRT);
                Renderer::ClearRenderTarget(NormalRT);
                Renderer::ClearRenderTarget(ColorRT);
                Renderer::ClearRenderTarget(ORMRT);
                Renderer::ClearRenderTarget(MeshIDRT);
                Renderer::ClearDepthStencil(DepthRT);
                
                Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ORMRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(MeshIDRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
            });
            
            ECS::World.system("GBufferSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                if(BFCIndirectCommands.Num() <= 0 && NCIndirectCommands.Num() <= 0)
                {
                    return;
                }
                
                if(CameraIsDirty)
                {
                    Renderer::UploadBuffer(GBufferSceneDataBuffer, &GBufferSceneData, sizeof(SGBufferSceneData));
                    CameraIsDirty = false;
                }

                GBufferRootConstants.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_CBV);
                GBufferRootConstants.MaterialAttributes = MaterialAttributesBuffer.GetIndex(SRV_CBV);
                GBufferRootConstants.SceneDataBuffer = GBufferSceneDataBuffer->GetIndex(SRV_CBV);

                Renderer::ResourceBarrier(WorldPositionRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(NormalRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ORMRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(MeshIDRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                
                // Back face culling pass
                if(BFCIndirectCommands.Num() > 0)
                {
                    Renderer::SetPipeline(BFCGBufferPipeline);
                    Renderer::PushConstants(&GBufferRootConstants, sizeof(GBufferRootConstants));
                    Renderer::BindRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT });
                    Renderer::BindDepthStencil(DepthRT);
                    Renderer::SetVertexBuffers(Renderer::RenderData.VertexBuffer.GetBuffer(), 1);
                    Renderer::SetIndexBuffer(Renderer::RenderData.IndexBuffer.GetBuffer());
                    Renderer::DrawIndirect(BFCIndirectCommands.Num(), BFCIndirectBuffer);
                }
                
                // No culling pass
                if(NCIndirectCommands.Num() > 0)
                {
                    Renderer::SetPipeline(NCGBufferPipeline);
                    Renderer::PushConstants(&GBufferRootConstants, sizeof(GBufferRootConstants));
                    Renderer::BindRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT });
                    Renderer::BindDepthStencil(DepthRT);
                    Renderer::SetVertexBuffers(Renderer::RenderData.VertexBuffer.GetBuffer(), 1);
                    Renderer::SetIndexBuffer(Renderer::RenderData.IndexBuffer.GetBuffer());
                    Renderer::DrawIndirect(NCIndirectCommands.Num(), NCIndirectBuffer);
                }
                
                Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ORMRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(MeshIDRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
            });

            ECS::World.system<>("RayTracingSystem").kind(flecs::OnDraw).each([&]
            {
                RayTracingRootConstants.LightsBuffer = LightsBuffer.GetIndex(SRV_CBV);
                RayTracingRootConstants.LightTransformsBuffer = LightTransformsBuffer.GetIndex(SRV_CBV);
                RayTracingRootConstants.SceneDataBuffer = RayTracingSceneDataBuffer->GetIndex(SRV_CBV);
                RayTracingRootConstants.TLAS = Renderer::RenderData.TLAS.GetIndex(SRV_CBV);
                RayTracingRootConstants.VertexBuffer = Renderer::RenderData.VertexBuffer.GetIndex(SRV_CBV);
                RayTracingRootConstants.IndexBuffer = Renderer::RenderData.IndexBuffer.GetIndex(SRV_CBV);
                RayTracingRootConstants.DrawCommandsBuffer = DrawCommandsBuffer.GetIndex(SRV_CBV);
                RayTracingRootConstants.MaterialBuffer = MaterialAttributesBuffer.GetIndex(SRV_CBV);
                
                Renderer::UploadBuffer(RayTracingSceneDataBuffer, &RayTracingSceneData, sizeof(RayTracingSceneData));

                //dispatching
                Renderer::ResourceBarrier(RadianceRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                Renderer::ResourceBarrier(ReflectionRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                Renderer::SetPipeline(RTPipeline);
                Renderer::PushConstants(&RayTracingRootConstants, sizeof(RayTracingRootConstants));
                Renderer::TraceRays(RTPipeline, Point3(RadianceRT->GetWidth(), RadianceRT->GetHeight(), 1));
                Renderer::ResourceBarrier(RadianceRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ReflectionRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            });

            ECS::World.system("DeferredRenderingSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                Renderer::ResourceBarrier(TargetRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ClearRenderTarget(TargetRT);
                Renderer::ResourceBarrier(TargetRT, RENDER_TARGET, UNORDERED_ACCESS);
                Renderer::SetPipeline(DeferredRenderingPipeline);
                auto mousePos = Input::GetMousePos();
                Point2 relativeMousePos = Renderer::GetEditorViewport()->TransformMousePosition(mousePos);
                DeferredRootConstants.MousePos = relativeMousePos;
                Renderer::PushConstants(&DeferredRootConstants, sizeof(DeferredRootConstants));
                
                Vector2 resolution = Vector2(TargetRT->GetWidth(), TargetRT->GetHeight());
                Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
                GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                
                Renderer::Compute(GroupCount);
                int hoveredEntityId = {};
                Renderer::DownloadBuffer(HoveredMeshesBuffer, &hoveredEntityId, sizeof(int));
                Editor::HoveredEntityID = hoveredEntityId - 1;
                Renderer::ResourceBarrier(TargetRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
            });
        }

        void Update(float deltaTime) override
        {
        }

        void OnResize(Vector2 size) override
        {
            auto colorRTIndex = ColorRT->GetIndex(SRV_CBV);
            auto radianceRTIndex = RadianceRT->GetIndex(SRV_CBV);
            auto reflectionRTIndex = ReflectionRT->GetIndex(SRV_CBV);
            
            RayTracingRootConstants.WorldPositionRT = WorldPositionRT->GetIndex(SRV_CBV);
            RayTracingRootConstants.NormalRT = NormalRT->GetIndex(SRV_CBV);
            RayTracingRootConstants.ColorRT = colorRTIndex;
            RayTracingRootConstants.ORMRT = ORMRT->GetIndex(SRV_CBV);
            RayTracingRootConstants.RadianceRT = radianceRTIndex;
            RayTracingRootConstants.ReflectionRT = reflectionRTIndex;
            
            DeferredRootConstants.AlbedoRT = colorRTIndex;
            DeferredRootConstants.MeshIDRT = MeshIDRT->GetIndex(SRV_CBV);
            DeferredRootConstants.RadianceRT = radianceRTIndex;
            DeferredRootConstants.TargetRT = TargetRT->GetIndex(SRV_CBV);
            DeferredRootConstants.SkyColorRT = SkyColorRT->GetIndex(SRV_CBV);
            DeferredRootConstants.DepthRTID = DepthRT->GetIndex(SRV_CBV);
        }
    };
}
