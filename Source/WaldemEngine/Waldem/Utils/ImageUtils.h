#pragma once
#include "Waldem/Renderer/TextureFormat.h"

namespace Waldem
{
#define CALCULATE_IMAGE_DATA_SIZE(width, height, depth, format) \
((width) * (height) * (depth) * GetBytesPerPixel(format))

    inline int GetBytesPerPixel(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::R8G8B8A8_SINT:
        case TextureFormat::R8G8B8A8_UINT:
        case TextureFormat::R8G8B8A8_TYPELESS:
        case TextureFormat::R8G8B8A8_SNORM:
        case TextureFormat::R8G8B8A8_UNORM_SRGB:
        case TextureFormat::R8G8B8A8_UNORM:
        case TextureFormat::D32_FLOAT:
        case TextureFormat::R32_SINT:
        case TextureFormat::R32_UINT:
        case TextureFormat::R32_TYPELESS:
        case TextureFormat::R32_FLOAT:
            return 4;
        case TextureFormat::R32G32B32_SINT:
        case TextureFormat::R32G32B32_UINT:
        case TextureFormat::R32G32B32_TYPELESS:
        case TextureFormat::R32G32B32_FLOAT:
            return 12;
        case TextureFormat::R8_SINT:
        case TextureFormat::R8_UINT:
        case TextureFormat::R8_TYPELESS:
        case TextureFormat::R8_SNORM:
            return 1;     // 1 byte per pixel
            case TextureFormat::D16_UNORM:
            case TextureFormat::R16_SINT:
            case TextureFormat::R16_UINT:
            case TextureFormat::R16_TYPELESS:
            case TextureFormat::R16_SNORM:
            case TextureFormat::R16_UNORM:
            case TextureFormat::R16_FLOAT:
                return 2;
        default: return 0; // Unknown format
        }
    }
    
    class Texture2D;
    class TextureDesc;

    class WALDEM_API ImageUtils
    {
    public:
        static bool TryLoadTexture(const Path& path, Texture2D*& outTexture, bool relative = true);
        static Texture2D* LoadTexture(const Path& path);
        static bool TryLoadTextureDesc(const Path& path, TextureDesc*& outTexture, bool relative = true);
    };
}
