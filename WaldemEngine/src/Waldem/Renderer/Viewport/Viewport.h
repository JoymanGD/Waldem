#pragma once
#include "Waldem/Renderer/FrameBuffer.h"
#include "Waldem/Renderer/GBuffer.h"

namespace Waldem
{
    using ResizeCallback = std::function<void(Point2 size)>;
    
    struct SScissorRect
    {
        int left;
        int top;
        int right;
        int bottom;
        
        SScissorRect() = default;
        SScissorRect(int left, int top, int right, int bottom) : left(left), top(top), right(right), bottom(bottom) {}
    };
    
    enum ViewportType : uint
    {
        MainViewport = 0,
        EditorViewport = 1,
        GameViewport = 2,
        CustomViewport = 3
    };

    class WALDEM_API SViewport
    {
    public:
        ViewportType Type;
        WString Name = "Viewport";
        Point2 Position = {};
        Point2 Size = { 1280, 720 };
        Point2 DepthRange = { 0, 1 };
        SFrameBuffer* FrameBuffer = nullptr;
        bool IsMouseOver = false;
    private:
        WArray<ResizeCallback> ResizeCallbacks;
        ResizeCallback ResizeFunction;
        bool PendingResize = false;
        Vector2 PendingResizeTarget;
        SGBuffer* GBuffer = nullptr;
        ECS::Entity LinkedCamera;

    public:
        SViewport() = default;
        SViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, int frameBufferSize, bool gbuffer = true);
        SViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, SFrameBuffer* frameBuffer, bool gbuffer = true);

        void SetViewport(Point2 position, Point2 size, Point2 depthRange);

        ECS::Entity GetLinkedCamera() const;

        bool TryGetLinkedCamera(ECS::Entity& outCamera);

        void LinkCamera(ECS::Entity camera);

        void UnlinkCamera();

        void RequestResize(Point2 size);

        void ApplyPendingResize();
        
        void Resize(Point2 size);

        void Move(Point2 position);

        void SetResizeFunction(const ResizeCallback& callback);

        void SubscribeOnResize(const ResizeCallback& callback);

        Point2 TransformMousePosition(Point2 mousePos);

        SGBuffer* GetGBuffer();
        
        RenderTarget* GetGBufferRenderTarget(GBufferRenderTarget rt);
    };
}
