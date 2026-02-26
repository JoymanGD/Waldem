#pragma once
#include "Waldem/Renderer/GraphicResource.h"

namespace Waldem
{
    class RenderTarget : public GraphicResource
    {
        WString Name;
        int Width = 0;
        int Height = 0;
        TextureFormat Format = TextureFormat::R8G8B8A8_SNORM;
    public:
        RenderTarget() { SetType(RTYPE_RenderTarget);}
        RenderTarget(WString name, int width, int height, TextureFormat format) : Name(name), Width(width), Height(height), Format(format) { SetType(RTYPE_RenderTarget); }
        virtual ~RenderTarget() {}
        
        const WString& GetName() { return Name; }
        int GetWidth() { return Width; }
        int GetHeight() { return Height; }
        void SetWidth(int width) { Width = width; }
        void SetHeight(int height) { Height = height; }
        TextureFormat GetFormat() { return Format; }
    };
}
