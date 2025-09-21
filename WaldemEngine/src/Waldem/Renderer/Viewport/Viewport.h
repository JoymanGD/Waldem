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
        Main = 0,
        Editor = 1,
        Game = 2,
        Custom = 3
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
        bool PendingResize = false;
        Vector2 PendingResizeTarget;
        SGBuffer* GBuffer = nullptr;
        ECS::Entity LinkedCamera;

    public:
        SViewport() = default;
        
        SViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, int frameBufferSize, bool gbuffer = true) : Type(type), Name(name), Position(position), Size(size), DepthRange(depthRange)
        {
            FrameBuffer = new SFrameBuffer(name, frameBufferSize, size);

            if(gbuffer)
            {
                GBuffer = new SGBuffer(size);
            }
        }
        
        SViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, SFrameBuffer* frameBuffer, bool gbuffer = true) : Type(type), Name(name), Position(position), Size(size), DepthRange(depthRange)
        {
            FrameBuffer = frameBuffer;

            if(gbuffer)
            {
                GBuffer = new SGBuffer(size);
            }
        }

        void SetViewport(Point2 position, Point2 size, Point2 depthRange)
        {
            Position = position;
            Size = size;
            DepthRange = depthRange;
        }

        ECS::Entity GetLinkedCamera() const
        {
            return LinkedCamera;
        }

        bool TryGetLinkedCamera(ECS::Entity& outCamera)
        {
            if (LinkedCamera.is_valid())
            {
                outCamera = LinkedCamera;
                return true;
            }
            return false;
        }

        void LinkCamera(ECS::Entity camera)
        {
            LinkedCamera = camera;
        }

        void UnlinkCamera()
        {
            LinkedCamera = {};
        }

        void RequestResize(Point2 size)
        {
            PendingResize = true;
            PendingResizeTarget = size;
        }

        void ApplyPendingResize()
        {
            if (PendingResize)
            {
                Resize(PendingResizeTarget);
                PendingResize = false;
            }
        }
        
        void Resize(Point2 size)
        {
            Size = size;

            if(GBuffer)
            {
                GBuffer->Resize(size);
            }
            
            for (auto& callback : ResizeCallbacks)
            {
                callback(size);
            }
        }

        void Move(Point2 position)
        {
            Position = position;
        }

        void SubscribeOnResize(const ResizeCallback& callback)
        {
            ResizeCallbacks.Add(callback);
        }

        Point2 TransformMousePosition(Point2 mousePos)
        {
            Point2 transformedPos = mousePos - Position;

            return transformedPos;
        }

        SGBuffer* GetGBuffer() { return GBuffer; }
        
        RenderTarget* GetGBufferRenderTarget(GBufferRenderTarget rt)
        {
            return GBuffer ? GBuffer->GetRenderTarget(rt) : nullptr;
        }
    };
}
