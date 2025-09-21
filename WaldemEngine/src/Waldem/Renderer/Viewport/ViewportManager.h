#pragma once
#include "Viewport.h"

namespace Waldem
{
    class WALDEM_API ViewportManager
    {
    private:
        inline static WArray<SViewport*> Viewports;
        inline static WArray<SViewport*> IterableViewports;
        
    public:
        ViewportManager() = default;

        static SViewport* CreateViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, int frameBufferSize, bool gbuffer = true, bool iterable = true)
        {
            auto viewport = new SViewport(type, name, position, size, depthRange, frameBufferSize, gbuffer);
            Viewports.Add(viewport);

            if(iterable)
            {
                IterableViewports.Add(viewport);
            }

            return viewport;
        }

        static SViewport* CreateViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, SFrameBuffer* frameBuffer, bool gbuffer = true, bool iterable = true)
        {
            auto viewport = new SViewport(type, name, position, size, depthRange, frameBuffer, gbuffer);
            Viewports.Add(viewport);

            if(iterable)
            {
                IterableViewports.Add(viewport);
            }

            return viewport;
        }

        static WArray<SViewport*>& GetViewports()
        {
            return Viewports;
        }

        static SViewport* GetMainViewport()
        {
            for(auto& viewport : Viewports)
            {
                if(viewport->Type == Main)
                {
                    return viewport;
                }
            }

            return nullptr;
        }

        static SViewport* GetEditorViewport()
        {
            for(auto& viewport : Viewports)
            {
                if(viewport->Type == Editor)
                {
                    return viewport;
                }
            }

            return nullptr;
        }

        static SViewport* GetGameViewport()
        {
            for(auto& viewport : Viewports)
            {
                if(viewport->Type == Game)
                {
                    return viewport;
                }
            }

            return nullptr;
        }

        static SViewport* GetCurrentViewport()
        {
            return Renderer::GetCurrentViewport();
        }

        static void ForEach(const std::function<void(SViewport*)>& func)
        {
            for(auto& viewport : IterableViewports)
            {
                func(viewport);
            }
        }
    };
}
