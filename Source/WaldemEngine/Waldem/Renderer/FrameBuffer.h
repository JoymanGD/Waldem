#pragma once
#include "RenderTarget.h"

namespace Waldem
{
    class WALDEM_API SFrameBuffer
    {
    public:
        uint Size = 0;
        
        SFrameBuffer() = default;
        virtual ~SFrameBuffer() {}
        SFrameBuffer(const SFrameBuffer&) = delete;
        SFrameBuffer& operator=(const SFrameBuffer&) = delete;
        
        SFrameBuffer(WArray<RenderTarget*>& renderTargets, RenderTarget* depth);
        SFrameBuffer(WString name, int size, Vector2 resolution);

        void Advance();

        void AddRenderTarget(RenderTarget* renderTarget);

        void SetDepth(RenderTarget* depth);

        void Resize(Vector2 size);

        void Destroy();

        RenderTarget* GetRenderTarget(uint index) const;

        RenderTarget* GetCurrentRenderTarget() const;

        RenderTarget* GetDepth() const;

        // Prefix increment: ++FrameBuffer
        SFrameBuffer& operator++()
        {
            ++Counter;
            if (Counter >= Size) Counter = 0;
            return *this;
        }

        // Postfix increment: FrameBuffer++
        SFrameBuffer& operator++(int)
        {
            ++(*this);  // Simply forward to prefix
            return *this;
        }
        
    private:
        WArray<RenderTarget*> RenderTargets;
        RenderTarget* Depth = nullptr;
        int Counter = 0;
    };
}
