#include <wdpch.h>
#include "FrameBuffer.h"

#include "Renderer.h"
#include "RenderTarget.h"

namespace Waldem
{
    SFrameBuffer::SFrameBuffer(WArray<RenderTarget*>& renderTargets, RenderTarget* depth)
    {
        Size = renderTargets.Num();
        
        RenderTargets.Resize(Size);
        
        for (int i = 0; i < Size; ++i)
        {
            RenderTargets[i] = renderTargets[i];
        }

        Depth = std::move(depth);
    }
    
    SFrameBuffer::SFrameBuffer(WString name, int size, Vector2 resolution)
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

    void SFrameBuffer::Advance()
    {
        ++(*this);
    }

    void SFrameBuffer::AddRenderTarget(RenderTarget* renderTarget)
    {
        RenderTargets.Add(renderTarget);
        
        ++Size; 
    }

    void SFrameBuffer::SetDepth(RenderTarget* depth)
    {
        if (Depth)
        {
            Renderer::Destroy(Depth);
            Depth = nullptr;
        }
        
        Depth = depth;
    }

    void SFrameBuffer::Resize(Vector2 size)
    {
        for (int i = 0; i < Size; ++i)
        {
            Renderer::ResizeRenderTarget(RenderTargets[i], size.x, size.y);
        }
        
        Renderer::ResizeRenderTarget(Depth, size.x, size.y);
    }

    void SFrameBuffer::Destroy()
    {
        for (int i = 0; i < Size; ++i)
        {
            Renderer::DestroyImmediate(RenderTargets[i]);
        }

        RenderTargets.Clear();

        Renderer::DestroyImmediate(Depth);
        Depth = nullptr;

        Size = 0;
        Counter = 0;
    }

    RenderTarget* SFrameBuffer::GetRenderTarget(uint index) const
    {
        if (index >= RenderTargets.Num())
        {
            return nullptr;
        }
        
        return RenderTargets[index];
    }

    RenderTarget* SFrameBuffer::GetCurrentRenderTarget() const
    {
        if (Counter < 0 || Counter >= RenderTargets.Num())
        {
            return nullptr;
        }
        
        return RenderTargets[Counter];
    }

    RenderTarget* SFrameBuffer::GetDepth() const
    {
        return Depth;
    }
}
