#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"

namespace Waldem
{
    class WALDEM_API ScreenQuadSystem : public ISystem
    {
        RenderTarget* TargetRT = nullptr;
        Pipeline* QuadDrawPipeline = nullptr;
        PixelShader* QuadDrawPixelShader = nullptr;
        RootSignature* QuadDrawRootSignature = nullptr;
        Quad FullscreenQuad = {};
        
    public:
        ScreenQuadSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            TargetRT = resourceManager->GetRenderTarget("TargetRT");
            WArray<GraphicResource> QuadDrawPassResources;
            QuadDrawPassResources.Add(GraphicResource("TargetRT", TargetRT, 0));
            QuadDrawRootSignature = Renderer::CreateRootSignature(QuadDrawPassResources);
            QuadDrawPixelShader = Renderer::LoadPixelShader("QuadDraw");
            QuadDrawPipeline = Renderer::CreateGraphicPipeline("QuadDrawPipeline",
                                                            QuadDrawRootSignature,
                                                            QuadDrawPixelShader,
                                                            { TextureFormat::R8G8B8A8_UNORM },
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            inputElementDescs);

            IsInitialized = true;
        }

        void Deinitialize() override
        {
            if(QuadDrawRootSignature) QuadDrawRootSignature->Destroy();
            if(QuadDrawPixelShader) QuadDrawPixelShader->Destroy();
            if(QuadDrawPipeline) QuadDrawPipeline->Destroy();
            IsInitialized = false;
        }

        void Update(float deltaTime) override
        {
            if(!IsInitialized)
                return;
            
            //Quad drawing pass
            Renderer::SetPipeline(QuadDrawPipeline);
            Renderer::SetRootSignature(QuadDrawRootSignature);
            Renderer::Draw(&FullscreenQuad);
        }
    };
}
