#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Viewport/Viewport.h"
#include "Waldem/Renderer/Model/Quad.h"

namespace Waldem
{
    struct ScreenQuadRootConstants
    {
        uint TargetRT;
    };
    
    class WALDEM_API ScreenQuadSystem : public ICoreSystem
    {
        RenderTarget* TargetRT = nullptr;
        Pipeline* QuadDrawPipeline = nullptr;
        PixelShader* QuadDrawPixelShader = nullptr;
        Quad FullscreenQuad = {};
        ScreenQuadRootConstants RootConstants;
        
    public:
        ScreenQuadSystem() {}
        
        void Initialize() override
        {
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            TargetRT = Renderer::GetRenderTarget("TargetRT");
            
            QuadDrawPixelShader = Renderer::LoadPixelShader("QuadDraw");
            QuadDrawPipeline = Renderer::CreateGraphicPipeline("QuadDrawPipeline",
                                                            QuadDrawPixelShader,
                                                            { TextureFormat::R8G8B8A8_UNORM },
                                                            TextureFormat::UNKNOWN,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            inputElementDescs);

            ECS::World.system("ScreenQuadSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    RootConstants.TargetRT = viewport->GetGBufferRenderTarget(Deferred)->GetIndex(SRV_CBV);

                    Renderer::BindRenderTargets(viewport->FrameBuffer->GetCurrentRenderTarget());
                    Renderer::BindDepthStencil(nullptr);
                    Renderer::SetPipeline(QuadDrawPipeline);
                    Renderer::PushConstants(&RootConstants, sizeof(ScreenQuadRootConstants));
                    Renderer::Draw(&FullscreenQuad);
                }
            });
        }
    };
}
