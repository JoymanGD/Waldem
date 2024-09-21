#pragma once

namespace Waldem
{
    class Texture2D
    {
    public:
        virtual ~Texture2D() {}
        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual std::string GetName() const = 0;
        virtual uint32_t* GetSlot() = 0;

        static Texture2D* Create(std::string name, int width, int height, int channels, const uint8_t* data, uint32_t slot);
    };
}