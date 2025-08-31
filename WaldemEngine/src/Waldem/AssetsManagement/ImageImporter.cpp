#include "wdpch.h"
#include "ImageImporter.h"
#include "stb_image.h"
#include "Waldem/Renderer/Texture.h"
#include <filesystem>
#include <fstream>
#include "Waldem/Engine.h"

namespace Waldem
{
    WArray<Asset*> CImageImporter::Import(const Path& from, Path& to, bool relative)
    {
        WArray<Asset*> assets;
        
        WString fileName = from.filename().string();
        
        int width, height, channels;
        unsigned char *data = stbi_load(from.string().c_str(), &width, &height, &channels, 0);
        if(data)
        {
            unsigned char* rgbaData = data;
            
            if(channels == 3)
            {
                rgbaData = new unsigned char[width * height * 4];

                for (int i = 0; i < width * height; ++i)
                {
                    rgbaData[i * 4 + 0] = data[i * 3 + 0];
                    rgbaData[i * 4 + 1] = data[i * 3 + 1];
                    rgbaData[i * 4 + 2] = data[i * 3 + 2];
                    rgbaData[i * 4 + 3] = 255;
                }
            }
            
            auto texDesc = new TextureDesc(fileName, width, height, 1, TextureFormat::R8G8B8A8_UNORM, rgbaData);
            assets.Add(texDesc);
            stbi_image_free(data);
        }
        else
        {
		    WD_CORE_ERROR("Failed to load Image from path: {0}", from);
        }

        return assets;
    }
}
