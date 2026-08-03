#pragma once
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Terrain.h"
#include "Waldem/ECS/Components/TerrainMeshComponent.h"
#include "Waldem/Renderer/Model/TerrainMesh.h"

namespace Waldem
{
    struct STerrainSceneData
    {
        Matrix4 View;
        Matrix4 Proj;
        Matrix4 InvProj;
        Matrix4 WorldTransform;
        uint HeightMapIndex;
        uint AlbedoIndex;
        int Resolution;
        float Height;
    };
    
    class WALDEM_API TerrainSystem : public ISystem
    {
    private:
        Pipeline* TerrainPipeline = nullptr;
        PixelShader* TerrainPixelShader = nullptr;
        STerrainSceneData SceneData;
        Buffer* TerrainSceneDataBuffer;
    public:
        TerrainSystem() {}
        
        void Initialize() override
        {
            TerrainPixelShader = Renderer::LoadPixelShader("Terrain");
            TerrainPipeline = Renderer::CreateGraphicPipeline("TerrainPipeline",
                                                            TerrainPixelShader,
                                                            { SGBuffer::GetFormat(WorldPosition), SGBuffer::GetFormat(Normal), SGBuffer::GetFormat(Color), SGBuffer::GetFormat(ORM), SGBuffer::GetFormat(MeshID) },
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);
            TerrainSceneDataBuffer = Renderer::CreateBuffer("TerrainSceneData", BufferType::StorageBuffer, sizeof(STerrainSceneData), sizeof(STerrainSceneData));

            Observer<Terrain>("OnTerrainAdd", flecs::OnAdd, [&](ECS::Entity entity, Terrain& terrain)
            {
               entity.set<TerrainMeshComponent>({MeshReference(new TerrainMesh(terrain.Resolution))}); 
            });
            
            Observer<Terrain>("OnTerrainSet", flecs::OnSet, [&](ECS::Entity entity, Terrain& terrain)
            {
                if(!terrain.Heightmap.IsEmpty() && !terrain.Heightmap.IsValid())
                {
                    terrain.Heightmap.LoadAsset();
                }
                
                if(!terrain.Albedo.IsEmpty() && !terrain.Albedo.IsValid())
                {
                    terrain.Albedo.LoadAsset();
                }
            });
            
            System<ECS::OnDraw, Terrain>("TerrainRenderinSystem", [&](ECS::Entity entity, Terrain& terrain)
            {
                auto viewport = Renderer::GetCurrentViewport();
                ECS::Entity linkedCamera;
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto& cameraComponent = linkedCamera.get<Camera>();
                    auto& cameraTransformComponent = linkedCamera.get<Transform>();
                    auto& transformComponent = entity.get<Transform>();
                    auto& terrainMeshComponent = entity.get<TerrainMeshComponent>();
                    
                    SceneData.Proj = cameraComponent.ProjectionMatrix;
                    SceneData.View = inverse(cameraTransformComponent.RenderMatrix);
                    SceneData.WorldTransform = transformComponent.RenderMatrix;
                    SceneData.InvProj = inverse(cameraComponent.ProjectionMatrix);
                    SceneData.Resolution = terrain.Resolution;
                    SceneData.Height = terrain.Height;
                    SceneData.HeightMapIndex = !terrain.Heightmap.IsEmpty() && terrain.Heightmap.IsValid() ? terrain.Heightmap.Texture->GetIndex(SRV_CBV) : 0;
                    SceneData.AlbedoIndex = !terrain.Albedo.IsEmpty() && terrain.Albedo.IsValid() ? terrain.Albedo.Texture->GetIndex(SRV_CBV) : 0;
                    
                    Renderer::UploadBuffer(TerrainSceneDataBuffer, &SceneData, sizeof(STerrainSceneData));
                    auto sceneDataBufferIndex = TerrainSceneDataBuffer->GetIndex(SRV_CBV);

                    auto gbuffer = viewport->GetGBuffer();
                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, ALL_SHADER_RESOURCE, RENDER_TARGET);
                    gbuffer->Barrier(Depth, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    
                    Renderer::SetPipeline(TerrainPipeline);
                    Renderer::PushConstants(&sceneDataBufferIndex, sizeof(uint));
                    Renderer::BindRenderTargets({ gbuffer->GetRenderTarget(WorldPosition), gbuffer->GetRenderTarget(Normal), gbuffer->GetRenderTarget(Color), gbuffer->GetRenderTarget(ORM), gbuffer->GetRenderTarget(MeshID) });
                    Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                    Renderer::Draw(terrainMeshComponent.MeshRef.Mesh);
                    
                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, RENDER_TARGET, ALL_SHADER_RESOURCE);
                    gbuffer->Barrier(Depth, DEPTH_WRITE, ALL_SHADER_RESOURCE);
                }
            });
        }
    };
}
