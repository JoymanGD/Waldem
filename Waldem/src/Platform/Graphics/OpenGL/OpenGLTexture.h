#pragma once
#include "Waldem/Renderer/Texture.h"

namespace Waldem
{
    class OpenGLTexture2D : public Texture2D
    {
    public:
        OpenGLTexture2D(std::string name, int width, int height, int channels, const uint8_t* data, uint32_t slot);
        ~OpenGLTexture2D() override;
        void Bind() const override;
        void Unbind() const override;
        std::string GetName() const override { return Name; }
        uint32_t* GetSlot() override { return &Slot; }

        std::string Name;
        uint32_t Slot;
        int Width;
        int Height;
        int Channels;
        uint32_t TextureID;
    };
}