#pragma once
#include "Texture.h"

namespace Waldem
{    
    class RenderTarget : public Texture2D
    {
    public:
        RenderTarget(String name, int width, int height, TextureFormat format) : Texture2D(name, width, height, format) {}
        virtual ~RenderTarget() {}
    };
}
