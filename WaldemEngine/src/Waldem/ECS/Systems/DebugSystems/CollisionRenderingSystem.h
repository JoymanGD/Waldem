#pragma once
#include <FlecsUtils.h>

#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/EditorCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Renderer/Model/Line.h"
#include "Waldem/Renderer/Model/LineMesh.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    struct SMeshColliderRenderingConstants
    {
        Matrix4 WorldViewProj;
        Vector4 Color;
    };
    
    class WALDEM_API CollisionRenderingSystem : public ISystem
    {
        RenderTarget* DepthRT = nullptr;
        
        //Mesh colliders rendering
        SMeshColliderRenderingConstants MeshColliderRenderingConstants;
        Pipeline* MeshCollidersRenderingPipeline = nullptr;
        PixelShader* MeshPixelShader = nullptr;
        Matrix4 ViewProjection;
        
    public:
        CollisionRenderingSystem() {}

        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {
            DepthRT = resourceManager->GetRenderTarget("DepthRT");
            
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 16, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            MeshPixelShader = Renderer::LoadPixelShader("SimpleMesh");

            RasterizerDesc rasterizerDesc = DEFAULT_RASTERIZER_DESC;
            rasterizerDesc.FillMode = WD_FILL_MODE_WIREFRAME;
            rasterizerDesc.DepthBias = -30;
            rasterizerDesc.DepthBiasClamp = 0;
            MeshCollidersRenderingPipeline = Renderer::CreateGraphicPipeline("MeshCollidersRenderingPipeline",
                                                            MeshPixelShader,
                                                            { TextureFormat::R8G8B8A8_UNORM },
                                                            TextureFormat::D32_FLOAT,
                                                            rasterizerDesc,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            ECS::World.system("CollisionRenderingSystem").kind(flecs::OnDraw).run([&](flecs::iter& it)
            {
                if(IsInitialized)
                {
                    if(auto editorCamera = ECS::World.lookup("EditorCamera"))
                    {
                        ViewProjection = editorCamera.get<Camera>()->ViewProjectionMatrix;
                    }
                            
                    Renderer::BindDepthStencil(DepthRT);
                    Renderer::ResourceBarrier(DepthRT, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    ECS::World.query<ColliderComponent, Transform>().each([&](ColliderComponent& collider, Transform& transform)
                    {
                        Vector4 color = collider.IsColliding ? Vector4(1.0f, 0.0f, 0.0f, 1.0f) : Vector4(0.0f, 1.0f, 0.0f, 1.0f);
                        
                        MeshColliderRenderingConstants.WorldViewProj = ViewProjection * transform.Matrix;
                        MeshColliderRenderingConstants.Color = color;
                        Renderer::SetPipeline(MeshCollidersRenderingPipeline);
                        Renderer::PushConstants(&MeshColliderRenderingConstants, sizeof(SMeshColliderRenderingConstants));
                        // Renderer::Draw(collider.MeshData.Mesh);
                    });
                }
                
                Renderer::ResourceBarrier(DepthRT, DEPTH_WRITE, ALL_SHADER_RESOURCE);
            });

            IsInitialized = true;
        }
    };
}
