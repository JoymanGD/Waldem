#include "wdpch.h"

#include <string_view>
#include <functional>
#include "ResourceManager.h"
#include <filesystem>
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    RenderTarget* ResourceManager::CreateRenderTarget(WString name, int width, int height, TextureFormat format)
    {
        RenderTarget* rt = Renderer::CreateRenderTarget(name, width, height, format);

        if(RenderTargets.Contains(name))
        {
            RenderTargets[name] = rt;
        }
        else
        {
            RenderTargets.Add(name, rt);
        }
        
        return rt;
    }
    
    Buffer* ResourceManager::CreateBuffer(WString name, BufferType type, void* data, uint32_t size, uint32_t stride)
    {
        Buffer* buffer = Renderer::CreateBuffer(name, type, data, size, stride);

        if(Buffers.Contains(name))
        {
            Buffers[name] = buffer;
        }
        else
        {
            Buffers.Add(name, buffer);
        }

        return buffer;
    }

    RenderTarget* ResourceManager::CloneRenderTarget(RenderTarget* renderTarget)
    {
        RenderTarget* destRT = CreateRenderTarget(renderTarget->GetName() + "_Clone", renderTarget->GetWidth(), renderTarget->GetHeight(), renderTarget->GetFormat());

        Renderer::ResourceBarrier(renderTarget, ALL_SHADER_RESOURCE, COPY_SOURCE);
        Renderer::ResourceBarrier(destRT, ALL_SHADER_RESOURCE, COPY_DEST);
        Renderer::CopyRenderTarget(destRT, renderTarget);
        Renderer::ResourceBarrier(renderTarget, COPY_SOURCE, ALL_SHADER_RESOURCE);
        Renderer::ResourceBarrier(destRT, COPY_DEST, ALL_SHADER_RESOURCE);

        return destRT;
    }

    Buffer* ResourceManager::CloneBuffer(Buffer* buffer)
    {
        Buffer* destBuffer = CreateBuffer(buffer->GetName() + "_Clone", buffer->GetType(), nullptr, buffer->GetSize(), buffer->GetStride());

        Renderer::ResourceBarrier(buffer, VERTEX_AND_CONSTANT_BUFFER | NON_PIXEL_SHADER_RESOURCE, COPY_SOURCE);
        Renderer::ResourceBarrier(destBuffer, VERTEX_AND_CONSTANT_BUFFER | NON_PIXEL_SHADER_RESOURCE, COPY_DEST);
        Renderer::CopyBuffer(destBuffer, buffer);
        Renderer::ResourceBarrier(buffer, COPY_SOURCE, VERTEX_AND_CONSTANT_BUFFER | NON_PIXEL_SHADER_RESOURCE);
        Renderer::ResourceBarrier(destBuffer, COPY_DEST, VERTEX_AND_CONSTANT_BUFFER | NON_PIXEL_SHADER_RESOURCE);

        return destBuffer;
    }

    Texture2D* ResourceManager::LoadTexture(const Path& path)
    {
        Texture2D* texture = ImageImporter.Import(path);
        auto name = texture->GetName();
        
        if(Textures.Contains(name))
        {
            Textures[name] = texture;
        }
        else
        {
            Textures.Add(name, texture);
        }

        return texture;
    }
}
