#pragma once
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/EditorComponent.h"
#include "Waldem/ECS/Components/Sky.h"
#include "Waldem/Renderer/ResizableAccelerationStructure.h"
#include "Waldem/Renderer/ResizableBuffer.h"

#define MAX_INDIRECT_COMMANDS 500

namespace Waldem
{
    struct GBufferSceneData
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
        int NumLights = 0;
    };

    struct RayTracingRootConstants
    {
        uint WorldPositionRT;
        uint NormalRT;
        uint ColorRT;
        uint ORMRT;
        uint OutputColorRT;
        uint LightsBuffer;
        uint LightTransformsBuffer;
        uint SceneDataBuffer; 
        uint TLAS;
    };
    
    struct DeferredRootConstants
    {
        Point2 MousePos;
        uint AlbedoRT;
        uint MeshIDRT;
        uint RadianceRT;
        uint TargetRT;
        uint SkyColorRT;
        uint HoveredMeshes;
    };

    struct SkySceneData
    {
        Matrix4 InverseProjection;
        Matrix4 InverseView;
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
        Texture2D* DummyTexture = nullptr;
        Buffer* DummyVertexBuffer = nullptr;
        Buffer* DummyIndexBuffer = nullptr;

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
        Pipeline* GBufferPipeline = nullptr;
        PixelShader* GBufferPixelShader = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        ResizableBuffer IndirectBuffer;
        ResizableBuffer VertexBuffer;
        ResizableBuffer IndexBuffer;
        ResizableBuffer WorldTransformsBuffer;
        ResizableBuffer MaterialAttributesBuffer;
        Buffer* GBufferSceneDataBuffer = nullptr;
        GBufferRootConstants GBufferRootConstants;
        bool CameraIsDirty = true;
        GBufferSceneData GBufferSceneData;
        WArray<IndirectCommand> IndirectCommands;
        WArray<MaterialShaderAttribute> MaterialAttributes;
        size_t VerticesCount = 0;
        size_t IndicesCount = 0;

        //RayTracing
        RenderTarget* RadianceRT = nullptr;
        Pipeline* RTPipeline = nullptr;
        RayTracingShader* RTShader = nullptr;
        WArray<AccelerationStructure*> BLAS;
        WMap<CMesh*, AccelerationStructure*> BLASToUpdate;
        RayTracingSceneData RayTracingSceneData;
        ResizableBuffer LightsBuffer;
        ResizableBuffer LightTransformsBuffer;
        Buffer* RayTracingSceneDataBuffer = nullptr;
        ResizableAccelerationStructure TLAS;
        RayTracingRootConstants RayTracingRootConstants;
        WArray<Matrix4> LightTransforms;

        //Deferred
        RenderTarget* TargetRT = nullptr;
        Pipeline* DeferredRenderingPipeline = nullptr;
        ComputeShader* DeferredRenderingComputeShader = nullptr;
        Point3 GroupCount;
        DeferredRootConstants DeferredRootConstants;
        Buffer* HoveredMeshesBuffer = nullptr;
        
    public:
        HybridRenderingSystem()
        {
            Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
            uint8_t* image_data = (uint8_t*)&dummyColor;

            DummyTexture = Renderer::CreateTexture("DummyTexture", 1, 1, TextureFormat::R8G8B8A8_UNORM, image_data);

            WArray DummyVertexData
            {
                Vertex(Vector3(0, 0, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
                Vertex(Vector3(0, 2, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
                Vertex(Vector3(2, 2, 0), Vector4(1, 1, 1, 1), Vector3(0, 0, 1), Vector3(0, 0, 0), Vector3(0, 0, 1), Vector2(0, 0), 0),
            };
            WArray DummyIndexData { 0, 1, 2 };
            DummyVertexBuffer = Renderer::CreateBuffer("DummyVertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), sizeof(Vertex));
            DummyIndexBuffer = Renderer::CreateBuffer("DummyIndexBuffer", BufferType::IndexBuffer, sizeof(uint), sizeof(uint));
        }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            //Sky
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            SceneDataBuffer = ResourceManager::CreateBuffer("SkySceneDataBuffer", BufferType::StorageBuffer, sizeof(SkyPassSceneData), sizeof(SkyPassSceneData));
            SkyPassRootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_UAV_CBV);
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            SkyColorRT = resourceManager->CreateRenderTarget("SkyColorRT", TargetRT->GetWidth(), TargetRT->GetHeight(), TargetRT->GetFormat());
            
            SkyPixelShader = Renderer::LoadPixelShader("Sky");
            SkyPipeline = Renderer::CreateGraphicPipeline("SkyPipeline",
                                                            SkyPixelShader,
                                                            { TextureFormat::R8G8B8A8_UNORM },
                                                            TextureFormat::UNKNOWN,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            inputElementDescs);
            
            //GBuffer
            IndirectBuffer = ResizableBuffer("IndirectDrawBuffer", BufferType::IndirectBuffer, sizeof(IndirectCommand), MAX_INDIRECT_COMMANDS);
            VertexBuffer = ResizableBuffer("VertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), 1000);
            IndexBuffer = ResizableBuffer("IndexBuffer", BufferType::IndexBuffer, sizeof(uint), 1000);
            WorldTransformsBuffer = ResizableBuffer("WorldTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), MAX_INDIRECT_COMMANDS);
            MaterialAttributesBuffer = ResizableBuffer("MaterialAttributesBuffer", BufferType::StorageBuffer, sizeof(MaterialShaderAttribute), MAX_INDIRECT_COMMANDS);
            GBufferSceneDataBuffer = ResourceManager::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(GBufferSceneData), sizeof(GBufferSceneData));
            
            WorldPositionRT = resourceManager->GetRenderTarget("WorldPositionRT");
            NormalRT = resourceManager->GetRenderTarget("NormalRT");
            ColorRT = resourceManager->GetRenderTarget("ColorRT");
            ORMRT = resourceManager->GetRenderTarget("ORMRT");
            MeshIDRT = resourceManager->GetRenderTarget("MeshIDRT");
            DepthRT = resourceManager->GetRenderTarget("DepthRT");
            
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            GBufferPipeline = Renderer::CreateGraphicPipeline("GBufferPipeline",
                                                            GBufferPixelShader,
                                                            { WorldPositionRT->GetFormat(), NormalRT->GetFormat(), ColorRT->GetFormat(), ORMRT->GetFormat(), MeshIDRT->GetFormat() },
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            //Raytracing
            RadianceRT = resourceManager->GetRenderTarget("RadianceRT");
            RTShader = Renderer::LoadRayTracingShader("RayTracing/Radiance");
            RTPipeline = Renderer::CreateRayTracingPipeline("RayTracingPipeline", RTShader);
            LightsBuffer = ResizableBuffer("LightsBuffer", StorageBuffer, sizeof(LightData), 40);
            LightTransformsBuffer = ResizableBuffer("LightTransformsBuffer", StorageBuffer, sizeof(Matrix4), 40);
            RayTracingSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", StorageBuffer, sizeof(RayTracingSceneData), sizeof(RayTracingSceneData), &RayTracingSceneData);
            TLAS = ResizableAccelerationStructure("RayTracingTLAS", 50);
            
            //Deferred
            HoveredMeshesBuffer = Renderer::CreateBuffer("HoveredMeshes", StorageBuffer, sizeof(int), sizeof(int));
            DeferredRootConstants.HoveredMeshes = HoveredMeshesBuffer->GetIndex(SRV_UAV_CBV);

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
                auto drawId = IdManager::AddId(entity, DrawIdType);
                
                if(drawId >= IndirectCommands.Num())
                {
                    IndirectCommands.Add(IndirectCommand());
                    IndirectBuffer.AddData(nullptr, sizeof(IndirectCommand));
                }
                
                WorldTransformsBuffer.AddData(&transform.Matrix, sizeof(Matrix4));

                if(drawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.AddData(nullptr, sizeof(MaterialShaderAttribute));
                }

                TLAS.AddEmptyData();
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnSet).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                int drawId;

                if(IdManager::GetId(entity, DrawIdType, drawId))
                {
                    bool meshReferenceIsEmpty = meshComponent.MeshRef.Reference.empty() || meshComponent.MeshRef.Reference == "Empty";
                    
                    if(meshReferenceIsEmpty && !meshComponent.MeshRef.IsValid())
                    {
                        return;
                    }

                    if(!meshReferenceIsEmpty && !meshComponent.MeshRef.IsValid())
                    {
                        meshComponent.MeshRef.LoadAsset(contentManager);
                    }
                    
                    if(meshComponent.MeshRef.IsValid())
                    {
                        auto& command = IndirectCommands[drawId];
                        command.DrawId = drawId;
                        command.DrawIndexed = {
                            (uint)meshComponent.MeshRef.Mesh->IndexData.Num(),
                            1,
                            (uint)IndicesCount,
                            (int)VerticesCount,
                            0
                        };  

                        IndirectBuffer.UpdateData(&command, sizeof(IndirectCommand), sizeof(IndirectCommand) * drawId);

                        VertexBuffer.AddData(meshComponent.MeshRef.Mesh->VertexData.GetData(), meshComponent.MeshRef.Mesh->VertexData.GetSize());
                        IndexBuffer.AddData(meshComponent.MeshRef.Mesh->IndexData.GetData(), meshComponent.MeshRef.Mesh->IndexData.GetSize());

                        VerticesCount += meshComponent.MeshRef.Mesh->VertexData.Num();
                        IndicesCount += meshComponent.MeshRef.Mesh->IndexData.Num();

                        auto& materialAttribute = MaterialAttributes[drawId];

                        materialAttribute.Albedo = meshComponent.MeshRef.Mesh->CurrentMaterial->Albedo;
                        materialAttribute.Metallic = meshComponent.MeshRef.Mesh->CurrentMaterial->Metallic;
                        materialAttribute.Roughness = meshComponent.MeshRef.Mesh->CurrentMaterial->Roughness;
                        
                        if(meshComponent.MeshRef.Mesh->CurrentMaterial->HasDiffuseTexture())
                        {
                            materialAttribute.DiffuseTextureID = meshComponent.MeshRef.Mesh->CurrentMaterial->GetDiffuseTexture()->GetIndex(SRV_UAV_CBV);
                            
                            if(meshComponent.MeshRef.Mesh->CurrentMaterial->HasNormalTexture())
                                materialAttribute.NormalTextureID = meshComponent.MeshRef.Mesh->CurrentMaterial->GetNormalTexture()->GetIndex(SRV_UAV_CBV);
                            if(meshComponent.MeshRef.Mesh->CurrentMaterial->HasORMTexture())
                                materialAttribute.ORMTextureID = meshComponent.MeshRef.Mesh->CurrentMaterial->GetORMTexture()->GetIndex(SRV_UAV_CBV);
                        }

                        MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), sizeof(MaterialShaderAttribute) * drawId);
                    }

                    auto transform = entity.get<Transform>();

                    TLAS.SetData(drawId, meshComponent, *transform);
                }
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                int drawId;

                if(IdManager::GetId(entity, DrawIdType, drawId))
                {
                    IndirectCommand& command = IndirectCommands[drawId];

                    if(meshComponent.MeshRef.IsValid())
                    {
                        VertexBuffer.RemoveData(meshComponent.MeshRef.Mesh->VertexData.GetSize(), command.DrawIndexed.BaseVertexLocation * sizeof(Vertex));
                        IndexBuffer.RemoveData(meshComponent.MeshRef.Mesh->IndexData.GetSize(), command.DrawIndexed.StartIndexLocation * sizeof(uint));

                        VerticesCount -= meshComponent.MeshRef.Mesh->VertexData.Num();
                        IndicesCount -= meshComponent.MeshRef.Mesh->IndexData.Num();
                    }
                    
                    IndirectBuffer.RemoveData(sizeof(IndirectCommand), drawId * sizeof(IndirectCommand));
                    
                    command.DrawId = -1;
                    command.DrawIndexed = { 0, 0, 0, 0, 0 };

                    MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), drawId * sizeof(MaterialShaderAttribute));

                    TLAS.RemoveData(drawId);
                    
                    auto transform = entity.get<Transform>();
                    
                    if(transform)
                    {
                        WorldTransformsBuffer.RemoveData(sizeof(Matrix4), drawId * sizeof(Matrix4));
                    }

                    IdManager::RemoveId(entity, DrawIdType);
                }
            });
            
            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform)
            {
                int drawId;

                if(IdManager::GetId(entity, DrawIdType, drawId))
                {
                    auto meshComponent = entity.get<MeshComponent>();
                    
                    if(meshComponent)
                    {
                        WorldTransformsBuffer.UpdateData(&transform.Matrix, sizeof(Matrix4), drawId * sizeof(Matrix4));
                        
                        TLAS.UpdateTransform(drawId, transform);
                    }
                }
                
                auto light = entity.get<Light>();

                if(light)
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
                SkyPassSceneData.InverseProjection = inverseProj;
                SkyPassSceneData.InverseView = world;
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
            
            ECS::World.system("GBufferSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                if(IndirectCommands.Num() <= 0)
                {
                    return;
                }
                
                if(CameraIsDirty)
                {
                    Renderer::UploadBuffer(GBufferSceneDataBuffer, &GBufferSceneData, sizeof(GBufferSceneData));
                    CameraIsDirty = false;
                }

                GBufferRootConstants.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_UAV_CBV);
                GBufferRootConstants.MaterialAttributes = MaterialAttributesBuffer.GetIndex(SRV_UAV_CBV);
                GBufferRootConstants.SceneDataBuffer = GBufferSceneDataBuffer->GetIndex(SRV_UAV_CBV);

                Renderer::ResourceBarrier(WorldPositionRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(NormalRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ColorRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(ORMRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(MeshIDRT, ALL_SHADER_RESOURCE, RENDER_TARGET);
                Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);

                Renderer::SetPipeline(GBufferPipeline);
                Renderer::PushConstants(&GBufferRootConstants, sizeof(GBufferRootConstants));
                Renderer::BindRenderTargets({ WorldPositionRT, NormalRT, ColorRT, ORMRT, MeshIDRT });
                Renderer::BindDepthStencil(DepthRT);
                Renderer::SetVertexBuffers(VertexBuffer, 1);
                Renderer::SetIndexBuffer(IndexBuffer);
                Renderer::DrawIndirect(IndirectCommands.Num(), IndirectBuffer);
                
                Renderer::ResourceBarrier(WorldPositionRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(NormalRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ColorRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(ORMRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(MeshIDRT, RENDER_TARGET, ALL_SHADER_RESOURCE);
                Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
            });

            ECS::World.system<>("RayTracingSystem").kind(flecs::OnDraw).each([&]
            {
                RayTracingRootConstants.LightsBuffer = LightsBuffer.GetIndex(SRV_UAV_CBV);
                RayTracingRootConstants.LightTransformsBuffer = LightTransformsBuffer.GetIndex(SRV_UAV_CBV);
                RayTracingRootConstants.SceneDataBuffer = RayTracingSceneDataBuffer->GetIndex(SRV_UAV_CBV);
                RayTracingRootConstants.TLAS = TLAS.GetIndex(SRV_UAV_CBV);
                
                Renderer::UploadBuffer(RayTracingSceneDataBuffer, &RayTracingSceneData, sizeof(RayTracingSceneData));

                //dispatching
                Renderer::ResourceBarrier(RadianceRT, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
                Renderer::SetPipeline(RTPipeline);
                Renderer::PushConstants(&RayTracingRootConstants, sizeof(RayTracingRootConstants));
                Renderer::TraceRays(RTPipeline, Point3(RadianceRT->GetWidth(), RadianceRT->GetHeight(), 1));
                Renderer::ResourceBarrier(RadianceRT, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
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
                int hoveredEntityId = 0;
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
            auto colorRTIndex = ColorRT->GetIndex(SRV_UAV_CBV);
            auto radianceRTIndex = RadianceRT->GetIndex(SRV_UAV_CBV);
            
            RayTracingRootConstants.WorldPositionRT = WorldPositionRT->GetIndex(SRV_UAV_CBV);
            RayTracingRootConstants.NormalRT = NormalRT->GetIndex(SRV_UAV_CBV);
            RayTracingRootConstants.ColorRT = colorRTIndex;
            RayTracingRootConstants.ORMRT = ORMRT->GetIndex(SRV_UAV_CBV);
            RayTracingRootConstants.OutputColorRT = radianceRTIndex;
            
            DeferredRootConstants.AlbedoRT = colorRTIndex;
            DeferredRootConstants.MeshIDRT = MeshIDRT->GetIndex(SRV_UAV_CBV);
            DeferredRootConstants.RadianceRT = radianceRTIndex;
            DeferredRootConstants.TargetRT = TargetRT->GetIndex(SRV_UAV_CBV);
            DeferredRootConstants.SkyColorRT = SkyColorRT->GetIndex(SRV_UAV_CBV);
        }
    };
}
