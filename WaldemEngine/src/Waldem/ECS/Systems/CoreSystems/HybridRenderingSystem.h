#pragma once
#include "Waldem/ECS/IdManager.h"
#include "Waldem/Input/Input.h"
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
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"

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
        uint MeshIDRT;
        uint RadianceRT;
        uint DeferredRT;
        uint SkyColorRT;
        uint HoveredMeshes;
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
        WArray<IndirectCommand> BFCIndirectCommands;
        ResizableBuffer NCIndirectBuffer; //No culling indirect buffer
        WArray<IndirectCommand> NCIndirectCommands;
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
        WArray<AccelerationStructure*> BLAS;
        WMap<CMesh*, AccelerationStructure*> BLASToUpdate;
        SRayTracingSceneData RayTracingSceneData;
        ResizableBuffer LightsBuffer;
        ResizableBuffer LightTransformsBuffer;
        Buffer* RayTracingSceneDataBuffer = nullptr;
        RayTracingRootConstants RayTracingRootConstants;
        WArray<Matrix4> LightTransforms;

        //Deferred
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
            LightsBuffer = ResizableBuffer("LightsBuffer", StorageBuffer, sizeof(LightData), 40);
            LightTransformsBuffer = ResizableBuffer("LightTransformsBuffer", StorageBuffer, sizeof(Matrix4), 40);
            RayTracingSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", StorageBuffer, sizeof(SRayTracingSceneData), sizeof(SRayTracingSceneData), &RayTracingSceneData);
            Renderer::RenderData.TLAS = ResizableAccelerationStructure("RayTracingTLAS", 50);
            
            //Deferred
            HoveredMeshesBuffer = Renderer::CreateBuffer("HoveredMeshes", StorageBuffer, sizeof(int), sizeof(int));
            DeferredRootConstants.HoveredMeshes = HoveredMeshesBuffer->GetIndex(UAV);

            DeferredRenderingComputeShader = Renderer::LoadComputeShader("DeferredRendering");
            DeferredRenderingPipeline = Renderer::CreateComputePipeline("DeferredLightingPipeline", DeferredRenderingComputeShader);
            
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

            ECS::World.observer<Sky>().event(flecs::OnSet).yield_existing().each([&](Sky& skybox)
            {
                SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
                SkyPassSceneData.SunDirection = Vector4(skybox.SunDirection, 1.0f);
            });

            ECS::World.system("SkyColorClearingSystem").kind(flecs::OnDraw).each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                viewport->GetGBuffer()->Clear({SkyColor});
            });
            
            ECS::World.system<Sky>("SkyRenderingSystem").kind(flecs::OnDraw).each([&](Sky& skybox)
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto skyColor = viewport->GetGBufferRenderTarget(SkyColor);
                    auto cameraComponent = linkedCamera.get<Camera>();
                    auto transformComponent = linkedCamera.get<Transform>();

                    SkyPassSceneData.InverseProjection = inverse(cameraComponent->ProjectionMatrix);
                    SkyPassSceneData.InverseView = transformComponent->Matrix;
                    SkyPassSceneData.ViewProjection = cameraComponent->ProjectionMatrix * inverse(transformComponent->Matrix);
                    SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                    SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                    SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
                    SkyPassSceneData.SunDirection = Vector4(skybox.SunDirection, 1.0f);
                    SkyPassSceneData.CameraPosition = Vector4(transformComponent->Position, 1.0f);
                    
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
            
            ECS::World.system("GBufferClearSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                SViewport* viewport = Renderer::GetCurrentViewport();
                viewport->GetGBuffer()->Clear({WorldPosition, Normal, Color, ORM, MeshID, Depth});
            });
            
            ECS::World.system("GBufferSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                if(BFCIndirectCommands.Num() <= 0 && NCIndirectCommands.Num() <= 0)
                {
                    return;
                }
                
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto cameraComponent = linkedCamera.get<Camera>();
                    auto transformComponent = linkedCamera.get<Transform>();
                    
                    GBufferSceneData.ProjectionMatrix = cameraComponent->ProjectionMatrix;
                    GBufferSceneData.ViewMatrix = inverse(transformComponent->Matrix);
                    GBufferSceneData.WorldMatrix = transformComponent->Matrix;
                    GBufferSceneData.InverseProjectionMatrix = inverse(cameraComponent->ProjectionMatrix);
                    
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

            ECS::World.system("RayTracingSystem").kind(flecs::OnDraw).each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto gbuffer = viewport->GetGBuffer();
                    auto radianceRT = gbuffer->GetRenderTarget(Radiance);

                    auto cameraComponent = linkedCamera.get<Camera>();
                    auto transformComponent = linkedCamera.get<Transform>();

                    RayTracingSceneData.CameraPosition = transformComponent->Position;
                    RayTracingSceneData.InvViewMatrix = transformComponent->Matrix;
                    RayTracingSceneData.InvProjectionMatrix = inverse(cameraComponent->ProjectionMatrix);
                    Renderer::UploadBuffer(RayTracingSceneDataBuffer, &RayTracingSceneData, sizeof(SRayTracingSceneData));

                    RayTracingRootConstants.WorldPositionRT = gbuffer->GetRenderTarget(WorldPosition)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.NormalRT = gbuffer->GetRenderTarget(Normal)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.ColorRT = gbuffer->GetRenderTarget(Color)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.ORMRT = gbuffer->GetRenderTarget(ORM)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.RadianceRT = radianceRT->GetIndex(SRV_CBV);
                    RayTracingRootConstants.ReflectionRT = gbuffer->GetRenderTarget(Reflection)->GetIndex(SRV_CBV);
                    RayTracingRootConstants.LightsBuffer = LightsBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.LightTransformsBuffer = LightTransformsBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.SceneDataBuffer = RayTracingSceneDataBuffer->GetIndex(SRV_CBV);
                    RayTracingRootConstants.TLAS = Renderer::RenderData.TLAS.GetIndex(SRV_CBV);
                    RayTracingRootConstants.VertexBuffer = Renderer::RenderData.VertexBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.IndexBuffer = Renderer::RenderData.IndexBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.DrawCommandsBuffer = DrawCommandsBuffer.GetIndex(SRV_CBV);
                    RayTracingRootConstants.MaterialBuffer = MaterialAttributesBuffer.GetIndex(SRV_CBV);

                    //dispatching
                    viewport->GetGBuffer()->Barriers({Radiance, Reflection}, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    Renderer::SetPipeline(RTPipeline);
                    Renderer::PushConstants(&RayTracingRootConstants, sizeof(RayTracingRootConstants));
                    Renderer::TraceRays(RTPipeline, Point3(radianceRT->GetWidth(), radianceRT->GetHeight(), 1));
                    viewport->GetGBuffer()->Barriers({Radiance, Reflection}, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });

            ECS::World.system("DeferredRenderingSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto gbuffer = viewport->GetGBuffer();
                    
                    auto deferredRT = viewport->GetGBufferRenderTarget(Deferred);
                    gbuffer->Clear({Deferred});
                    gbuffer->Barrier(Deferred, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                    
                    DeferredRootConstants.MeshIDRT = gbuffer->GetRenderTarget(MeshID)->GetIndex(SRV_CBV);
                    DeferredRootConstants.RadianceRT = gbuffer->GetRenderTarget(Radiance)->GetIndex(SRV_CBV);
                    DeferredRootConstants.DeferredRT = deferredRT->GetIndex(SRV_CBV);
                    DeferredRootConstants.SkyColorRT = gbuffer->GetRenderTarget(SkyColor)->GetIndex(SRV_CBV);
                    Renderer::SetPipeline(DeferredRenderingPipeline);
                    auto mousePos = Input::GetMousePos();
                    Point2 relativeMousePos = viewport->TransformMousePosition(mousePos);
                    DeferredRootConstants.MousePos = relativeMousePos;
                    Renderer::PushConstants(&DeferredRootConstants, sizeof(DeferredRootConstants));
                    
                    Vector2 resolution = Vector2(deferredRT->GetWidth(), deferredRT->GetHeight());
                    Point3 numThreads = Renderer::GetNumThreadsPerGroup(DeferredRenderingComputeShader);
                    GroupCount = Point3((resolution.x + numThreads.x - 1) / numThreads.x, (resolution.y + numThreads.y - 1) / numThreads.y, 1);
                    
                    Renderer::Compute(GroupCount);
                    int hoveredEntityId = {};
                    Renderer::DownloadBuffer(HoveredMeshesBuffer, &hoveredEntityId, sizeof(int));
                    Editor::HoveredEntityID = hoveredEntityId - 1;
                    viewport->GetGBuffer()->Barrier(Deferred, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}