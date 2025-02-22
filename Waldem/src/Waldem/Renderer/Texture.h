#pragma once
#include "TextureFormat.h"

namespace Waldem
{
    class Texture2D
    {
    public:
        Texture2D(String name, int width, int height, TextureFormat format) : Name(name), Width(width), Height(height), Format(format) {}
        virtual ~Texture2D() {}
        virtual String GetName() { return Name; }
        virtual void* GetPlatformResource() = 0;
        TextureFormat GetFormat() { return Format; }
        int GetWidth() { return Width; }
        int GetHeight() { return Height; }
    protected:
        String Name;
        int Width;
        int Height;
        TextureFormat Format;
    };
}
