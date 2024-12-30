#include "wdpch.h"
#include "ResourceManager.h"

#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    RenderTarget* ResourceManager::CreateRenderTarget(String name, int width, int height, TextureFormat format)
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

    RenderTarget* ResourceManager::CloneRenderTarget(RenderTarget* renderTarget)
    {
        RenderTarget* destRT = CreateRenderTarget(renderTarget->GetName() + "_Clone", renderTarget->GetWidth(), renderTarget->GetHeight(), renderTarget->GetFormat());

        Renderer::CopyRenderTarget(destRT, renderTarget);

        return destRT;
    }
}
