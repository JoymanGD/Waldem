#pragma once
#include "Viewport.h"

namespace Waldem
{
    class WALDEM_API ViewportManager
    {
    private:
        inline static WArray<SViewport*> Viewports;
        inline static WArray<SViewport*> IterableViewports;
        inline static SViewport* GameViewportRef = nullptr;
        inline static SViewport* EditorViewportRef = nullptr;
        inline static SViewport* MainViewportRef = nullptr;
        
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

            switch (type)
            {
            case MainViewport:
                MainViewportRef = viewport;
                break;
            case EditorViewport:
                EditorViewportRef = viewport;
                break;
            case GameViewport:
                GameViewportRef = viewport;
                break;
            case CustomViewport:
                break;
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

            switch (type)
            {
            case MainViewport:
                MainViewportRef = viewport;
                break;
            case EditorViewport:
                EditorViewportRef = viewport;
                break;
            case GameViewport:
                GameViewportRef = viewport;
                break;
            case CustomViewport:
                break;
            }

            return viewport;
        }

        static WArray<SViewport*>& GetViewports()
        {
            return Viewports;
        }

        static SViewport* GetMainViewport()
        {
            return MainViewportRef;
        }

        static SViewport* GetEditorViewport()
        {
            return EditorViewportRef;
        }

        static SViewport* GetGameViewport()
        {
            return GameViewportRef ? GameViewportRef : MainViewportRef;
        }

        static SViewport* GetCurrentViewport()
        {
            return Renderer::GetCurrentViewport();
        }
    };
}
