#pragma once
#include "System.h"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    class WALDEM_API ScreenQuadSystem : ISystem
    {
        RenderTarget* TargetRT = nullptr;
        Pipeline* QuadDrawPipeline = nullptr;
        PixelShader* QuadDrawPixelShader = nullptr;
        RootSignature* QuadDrawRootSignature = nullptr;
        Quad FullscreenQuad = {};
        
    public:
        ScreenQuadSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            Vector2 resolution = Vector2(sceneData->Window->GetWidth(), sceneData->Window->GetHeight());
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            WArray<Resource> QuadDrawPassResources;
            QuadDrawPassResources.Add(Resource("TargetRT", TargetRT, 0));
            QuadDrawRootSignature = Renderer::CreateRootSignature(QuadDrawPassResources);
            QuadDrawPixelShader = Renderer::LoadPixelShader("QuadDraw");
            QuadDrawPipeline = Renderer::CreateGraphicPipeline("QuadDrawPipeline", { TextureFormat::R8G8B8A8_UNORM }, WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, QuadDrawRootSignature, QuadDrawPixelShader);
        }

        void Update(float deltaTime) override
        {
            //Quad drawing pass
            Renderer::SetPipeline(QuadDrawPipeline);
            Renderer::SetRootSignature(QuadDrawRootSignature);
            Renderer::Draw(&FullscreenQuad);
        }
    };
}
