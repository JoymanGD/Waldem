#pragma once
#include "FrameBuffer.h"

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

    class WALDEM_API SViewport
    {
    public:
        Point2 Position = {};
        Point2 Size = { 1280, 720 };
        Point2 DepthRange = { 0, 1 };
        SFrameBuffer* FrameBuffer = nullptr;
        bool IsMouseOver = false;
    private:
        WArray<ResizeCallback> ResizeCallbacks;
        bool PendingResize = false;
        Vector2 PendingResizeTarget;

    public:
        SViewport() = default;
        
        SViewport(WString name, Point2 position, Point2 size, Point2 depthRange, int frameBufferSize) : Position(position), Size(size), DepthRange(depthRange)
        {
            FrameBuffer = new SFrameBuffer(name, frameBufferSize, size);
        }
        
        SViewport(Point2 position, Point2 size, Point2 depthRange, SFrameBuffer* frameBuffer) : Position(position), Size(size), DepthRange(depthRange)
        {
            FrameBuffer = frameBuffer;
        }

        void SetViewport(Point2 position, Point2 size, Point2 depthRange)
        {
            Position = position;
            Size = size;
            DepthRange = depthRange;
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
    };
}
