#include "wdpch.h"
#include "ImageImporter.h"
#include "Waldem/Renderer/Texture.h"
#include <filesystem>
#include "Waldem/Engine.h"

namespace Waldem
{
    WArray<Asset*> CImageImporter::ImportTo(const Path& from, const Path& to, bool relative)
    {
        WArray<Asset*> assets;

        TextureDesc* desc = nullptr;
        if(ImageUtils::TryLoadTextureDesc(from, desc))
        {
            assets.Add(desc);
        }

        return assets;
    }
}
