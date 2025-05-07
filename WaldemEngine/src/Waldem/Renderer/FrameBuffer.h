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
        
        SFrameBuffer(WArray<RenderTarget*>& renderTargets, RenderTarget* depth)
        {
            Size = renderTargets.Num();
            
            RenderTargets.Resize(Size);
            
            for (int i = 0; i < Size; ++i)
            {
                RenderTargets[i] = renderTargets[i];
            }

            Depth = std::move(depth);
        }
        
        SFrameBuffer(WString name, int size, Vector2 resolution)
        {
            Size = size;
            
            WString postfix = "";
            
            RenderTargets.Resize(Size);
            
            for (int i = 0; i < Size; ++i)
            {
                postfix = "_FrameBuffer_" + std::to_string(i);
                RenderTargets[i] = Renderer::CreateRenderTarget(name + postfix, resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
            }

            Depth = Renderer::CreateRenderTarget(name + "_Depth", resolution.x, resolution.y, TextureFormat::D32_FLOAT);
        }

        void Advance()
        {
            ++(*this);
        }

        void AddRenderTarget(RenderTarget* renderTarget)
        {
            RenderTargets.Add(renderTarget);
            
            ++Size;
        }

        void SetDepth(RenderTarget* depth)
        {
            if (Depth)
            {
                Depth->Destroy();
                Depth = nullptr;
            }
            
            Depth = depth;
        }

        void Destroy()
        {
            for (int i = 0; i < Size; ++i)
            {
                RenderTargets[i]->Destroy();
            }

            RenderTargets.Clear();

            Depth->Destroy();
            Depth = nullptr;

            Size = 0;
            Counter = 0;
        }

        RenderTarget* GetRenderTarget(uint index) const
        {
            if (index >= RenderTargets.Num())
            {
                return nullptr;
            }
            
            return RenderTargets[index];
        }

        RenderTarget* GetCurrentRenderTarget() const
        {
            if (Counter < 0 || Counter >= RenderTargets.Num())
            {
                return nullptr;
            }
            
            return RenderTargets[Counter];
        }

        RenderTarget* GetDepth() const
        {
            return Depth;
        }

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
