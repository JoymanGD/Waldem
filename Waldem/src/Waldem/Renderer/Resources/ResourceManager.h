#pragma once
#include "Waldem/Import/ImageImporter.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Renderer/RenderTarget.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    class WALDEM_API ResourceManager
    {
    public:
        ResourceManager() = default;
        
        RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format);
        Buffer* CreateBuffer(String name, BufferType type, void* data, uint32_t size);
        RenderTarget* CloneRenderTarget(RenderTarget* renderTarget);
        Buffer* CloneBuffer(Buffer* buffer);
        Texture2D* LoadTexture(String path);
        RenderTarget* GetRenderTarget(String name) { return RenderTargets[name]; }
        Texture2D* GetTexture(String name) { return Textures[name]; }
        Buffer* GetBuffer(String name) { return Buffers[name]; }
        
    private:
        WMap<String, RenderTarget*> RenderTargets;
        WMap<String, Texture2D*> Textures;
        WMap<String, Buffer*> Buffers;

        ImageImporter ImageImporter;
    };
}
