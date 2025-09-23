#pragma once

#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class RenderTarget;

    enum GBufferRenderTarget
    {
        Deferred = 0,
        WorldPosition = 1,
        Normal = 2,
        Color = 3,
        ORM = 4,
        MeshID = 5,
        Depth = 6,
        Radiance = 7,
        Reflection = 8,
        SkyColor = 9,
        Ping = 10,
        Pong = 11,
    };

    struct SGBuffer
    {
        RenderTarget* DeferredRT = nullptr;
        RenderTarget* WorldPositionRT = nullptr;
        RenderTarget* NormalRT = nullptr;
        RenderTarget* ColorRT = nullptr;
        RenderTarget* ORMRT = nullptr;
        RenderTarget* MeshIDRT = nullptr;
        RenderTarget* DepthRT = nullptr;
        RenderTarget* RadianceRT = nullptr;
        RenderTarget* ReflectionRT = nullptr;
        RenderTarget* SkyColorRT = nullptr;
        RenderTarget* PingRT = nullptr;
        RenderTarget* PongRT = nullptr;

        SGBuffer(Vector2 size)
        {
            DeferredRT = Renderer::CreateRenderTarget("DeferredRT", size.x, size.y, GetFormat(GBufferRenderTarget::Deferred));
            WorldPositionRT = Renderer::CreateRenderTarget("WorldPositionRT", size.x, size.y, GetFormat(GBufferRenderTarget::WorldPosition));
            NormalRT = Renderer::CreateRenderTarget("NormalRT", size.x, size.y, GetFormat(GBufferRenderTarget::Normal));
            ColorRT = Renderer::CreateRenderTarget("ColorRT", size.x, size.y, GetFormat(GBufferRenderTarget::Color));
            ORMRT = Renderer::CreateRenderTarget("ORMRT", size.x, size.y, GetFormat(GBufferRenderTarget::ORM));
            MeshIDRT = Renderer::CreateRenderTarget("MeshIDRT", size.x, size.y, GetFormat(GBufferRenderTarget::MeshID));
            DepthRT = Renderer::CreateRenderTarget("DepthRT", size.x, size.y, GetFormat(GBufferRenderTarget::Depth));
            RadianceRT = Renderer::CreateRenderTarget("RadianceRT", size.x, size.y, GetFormat(GBufferRenderTarget::Radiance));
            ReflectionRT = Renderer::CreateRenderTarget("ReflectionRT", size.x, size.y, GetFormat(GBufferRenderTarget::Reflection));
            SkyColorRT = Renderer::CreateRenderTarget("SkyColorRT", size.x, size.y, GetFormat(GBufferRenderTarget::SkyColor));
            PingRT = Renderer::CreateRenderTarget("PingRT", size.x, size.y, GetFormat(GBufferRenderTarget::Ping));
            PongRT = Renderer::CreateRenderTarget("PongRT", size.x, size.y, GetFormat(GBufferRenderTarget::Pong));
        }

        RenderTarget* GetRenderTarget(GBufferRenderTarget rt)
        {
            switch (rt)
            {
            case GBufferRenderTarget::Deferred:
                return DeferredRT;
            case GBufferRenderTarget::WorldPosition:
                return WorldPositionRT;
            case GBufferRenderTarget::Normal:
                return NormalRT;
            case GBufferRenderTarget::Color:
                return ColorRT;
            case GBufferRenderTarget::ORM:
                return ORMRT;
            case GBufferRenderTarget::MeshID:
                return MeshIDRT;
            case GBufferRenderTarget::Depth:
                return DepthRT;
            case GBufferRenderTarget::Radiance:
                return RadianceRT;
            case GBufferRenderTarget::Reflection:
                return ReflectionRT;
            case GBufferRenderTarget::SkyColor:
                return SkyColorRT;
            case GBufferRenderTarget::Ping:
                return PingRT;
            case GBufferRenderTarget::Pong:
                return PongRT;
            default: return nullptr;
            }
        }

        void Resize(Vector2 size)
        {
            Renderer::ResizeRenderTarget(DeferredRT, size.x, size.y);
            Renderer::ResizeRenderTarget(WorldPositionRT, size.x, size.y);
            Renderer::ResizeRenderTarget(NormalRT, size.x, size.y);
            Renderer::ResizeRenderTarget(ColorRT, size.x, size.y);
            Renderer::ResizeRenderTarget(ORMRT, size.x, size.y);
            Renderer::ResizeRenderTarget(MeshIDRT, size.x, size.y);
            Renderer::ResizeRenderTarget(DepthRT, size.x, size.y);
            Renderer::ResizeRenderTarget(RadianceRT, size.x, size.y);
            Renderer::ResizeRenderTarget(ReflectionRT, size.x, size.y);
            Renderer::ResizeRenderTarget(SkyColorRT, size.x, size.y);
            Renderer::ResizeRenderTarget(PingRT, size.x, size.y);
            Renderer::ResizeRenderTarget(PongRT, size.x, size.y);
        }

        void Clear()
        {
            WArray rts = {
                GBufferRenderTarget::Deferred,
                GBufferRenderTarget::WorldPosition,
                GBufferRenderTarget::Normal,
                GBufferRenderTarget::Color,
                GBufferRenderTarget::ORM,
                GBufferRenderTarget::MeshID,
                GBufferRenderTarget::Depth,
                GBufferRenderTarget::Radiance,
                GBufferRenderTarget::Reflection,
                GBufferRenderTarget::SkyColor,
                GBufferRenderTarget::Ping,
                GBufferRenderTarget::Pong
            };
            Clear(rts);
        }

        void Clear(const WArray<GBufferRenderTarget>& rts)
        {
            for (auto& rt : rts)
            {
                auto renderTarget = GetRenderTarget(rt);

                if(rt == GBufferRenderTarget::Depth)
                {
                    Renderer::ResourceBarrier(renderTarget, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    Renderer::ClearDepthStencil(renderTarget);
                    Renderer::ResourceBarrier(renderTarget, DEPTH_WRITE, ALL_SHADER_RESOURCE);
                }
                else
                {
                    Renderer::ResourceBarrier(renderTarget, ALL_SHADER_RESOURCE, RENDER_TARGET);
                    Renderer::ClearRenderTarget(renderTarget);
                    Renderer::ResourceBarrier(renderTarget, RENDER_TARGET, ALL_SHADER_RESOURCE);
                }
            }
        }

        void Barrier(GBufferRenderTarget rt, ResourceStates before, ResourceStates after)
        {
            auto renderTarget = GetRenderTarget(rt);
            Renderer::ResourceBarrier(renderTarget, before, after);
        }

        void Barriers(const WArray<GBufferRenderTarget>& rts, ResourceStates before, ResourceStates after)
        {
            for (auto& rt : rts)
            {
                Barrier(rt, before, after);
            }
        }

        static TextureFormat GetFormat(GBufferRenderTarget rt)
        {
            switch (rt)
            {
                case GBufferRenderTarget::Deferred: return TextureFormat::R16G16B16A16_FLOAT;
                case GBufferRenderTarget::WorldPosition: return TextureFormat::R32G32B32A32_FLOAT;
                case GBufferRenderTarget::Normal: return TextureFormat::R16G16B16A16_FLOAT;
                case GBufferRenderTarget::Color: return TextureFormat::R8G8B8A8_UNORM;
                case GBufferRenderTarget::ORM: return TextureFormat::R32G32B32A32_FLOAT;
                case GBufferRenderTarget::MeshID: return TextureFormat::R16G16_SINT;
                case GBufferRenderTarget::Depth: return TextureFormat::D32_FLOAT;
                case GBufferRenderTarget::Radiance: return TextureFormat::R32G32B32A32_FLOAT;
                case GBufferRenderTarget::Reflection: return TextureFormat::R32G32B32A32_FLOAT;
                case GBufferRenderTarget::SkyColor: return TextureFormat::R8G8B8A8_UNORM;
                case GBufferRenderTarget::Ping: return TextureFormat::R16G16B16A16_FLOAT;
                case GBufferRenderTarget::Pong: return TextureFormat::R16G16B16A16_FLOAT;
                default: return TextureFormat::UNKNOWN;
            }
        }

        static WArray<TextureFormat> GetFormats()
        {
            return { TextureFormat::R16G16B16A16_FLOAT, TextureFormat::R32G32B32A32_FLOAT, TextureFormat::R16G16B16A16_FLOAT, TextureFormat::R8G8B8A8_UNORM, TextureFormat::R32G32B32A32_FLOAT, TextureFormat::R16G16_SINT, TextureFormat::D32_FLOAT, TextureFormat::R32G32B32A32_FLOAT, TextureFormat::R32G32B32A32_FLOAT, TextureFormat::R8G8B8A8_UNORM, TextureFormat::R16G16B16A16_FLOAT, TextureFormat::R16G16B16A16_FLOAT };
        }
    };
}
