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
        uint ViewMode;
        uint PathTracingEnabled;
    };
    
    class WALDEM_API ScreenQuadSystem : public ICoreSystem
    {
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
                    auto gbuffer = viewport->GetGBuffer();
                    uint selectedOutputTarget = viewport->Type == EditorViewport
                        ? Renderer::RenderData.EditorViewportOutputTarget
                        : Renderer::RenderData.GameViewportOutputTarget;

                    RootConstants.ViewMode = selectedOutputTarget;
                    RootConstants.PathTracingEnabled = Renderer::RenderData.FeatureToggles.EnablePathTracing ? 1u : 0u;

                    if(selectedOutputTarget == 1)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Normal)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 2)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Reflection)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 3)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(WorldPosition)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 4)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(ORM)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 5)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(MeshID)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 6)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Color)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 7)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Radiance)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 8)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(NIVIrradiance)->GetIndex(SRV_CBV);
                    }
                    else if(selectedOutputTarget == 9)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(PathTracing)->GetIndex(SRV_CBV);
                    }
                    else if(Renderer::RenderData.FeatureToggles.EnableDeferredPass)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Deferred)->GetIndex(SRV_CBV);
                    }
                    else if(Renderer::RenderData.FeatureToggles.EnableRayTracingPass)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Radiance)->GetIndex(SRV_CBV);
                    }
                    else if(Renderer::RenderData.FeatureToggles.EnableGBufferPass)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Color)->GetIndex(SRV_CBV);
                    }
                    else if(Renderer::RenderData.FeatureToggles.EnableSkyPass)
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(SkyColor)->GetIndex(SRV_CBV);
                    }
                    else
                    {
                        RootConstants.TargetRT = gbuffer->GetRenderTarget(Deferred)->GetIndex(SRV_CBV);
                    }

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


