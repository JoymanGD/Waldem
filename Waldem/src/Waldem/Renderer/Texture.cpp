#include "wdpch.h"
#include "Texture.h"
#include "Renderer.h"
#include "Platform/Graphics/OpenGL/OpenGLTexture.h"

namespace Waldem
{
    Texture2D* Texture2D::Create(std::string name, int width, int height, int channels, const uint8_t* data, uint32_t slot)
    {
        switch (Renderer::RAPI)
        {
        case RendererAPI::None:
            WD_CORE_ASSERT(false, "RendererAPI 'None' is not supported!")
            return nullptr;
        case RendererAPI::OpenGL:
            return new OpenGLTexture2D(name, width, height, channels, data, slot);
        default:
            WD_CORE_ASSERT(false, "The API is not supported as an Rendering API")
            return nullptr;
        }
    }
}
