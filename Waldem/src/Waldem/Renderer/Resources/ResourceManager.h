#pragma once
#include "Waldem/Renderer/RenderTarget.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    class WALDEM_API ResourceManager
    {
    public:
        ResourceManager() = default;
        
        RenderTarget* CreateRenderTarget(String name, int width, int height, TextureFormat format);
        RenderTarget* GetRenderTarget(String name) { return RenderTargets[name]; }
        
    private:
        WMap<String, RenderTarget*> RenderTargets;
    };
}
