#pragma once
#include "FrameBuffer.h"

namespace Waldem
{
    struct SScissorRect
    {
        float left;
        float top;
        float right;
        float bottom;
        
        SScissorRect() = default;
        SScissorRect(float left, float top, float right, float bottom) : left(left), top(top), right(right), bottom(bottom) {}
    };

    class WALDEM_API SViewport
    {
    public:
        Vector2 Position = {};
        Vector2 Resolution = { 1280, 720 };
        Vector2 DepthRange = { 0, 1 };
        SFrameBuffer* FrameBuffer = nullptr;
        SScissorRect ScissorRect = { Position.x, Position.y, Resolution.x, Resolution.y };

        SViewport() = default;
        
        SViewport(WString name, Vector2 position, Vector2 resolution, Vector2 depthRange, int frameBufferSize, bool fullScissor = true) : Position(position), Resolution(resolution), DepthRange(depthRange)
        {
            if(fullScissor)
            {
                ScissorRect.left = position.x;
                ScissorRect.top = position.y;
                ScissorRect.right = position.x + resolution.x;
                ScissorRect.bottom = position.y + resolution.y;
            }

            FrameBuffer = new SFrameBuffer(name, frameBufferSize, resolution);
        }
        
        SViewport(Vector2 position, Vector2 resolution, Vector2 depthRange, SFrameBuffer* frameBuffer, bool fullScissor = true) : Position(position), Resolution(resolution), DepthRange(depthRange)
        {
            if(fullScissor)
            {
                ScissorRect.left = position.x;
                ScissorRect.top = position.y;
                ScissorRect.right = position.x + resolution.x;
                ScissorRect.bottom = position.y + resolution.y;
            }

            FrameBuffer = frameBuffer;
        }

        void SetViewport(Vector2 position, Vector2 size, Vector2 depthRange, bool fullScissor = true)
        {
            Position = position;
            Resolution = size;
            DepthRange = depthRange;
            
            if(fullScissor)
            {
                ScissorRect.left = position.x;
                ScissorRect.top = position.y;
                ScissorRect.right = position.x + size.x;
                ScissorRect.bottom = position.y + size.y;
            }
        }

        void SetScissorRect(float left, float top, float right, float bottom)
        {
            ScissorRect.left = left;
            ScissorRect.top = top;
            ScissorRect.right = right;
            ScissorRect.bottom = bottom;
        }
    };
}
