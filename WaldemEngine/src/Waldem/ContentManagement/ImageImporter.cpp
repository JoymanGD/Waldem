#include "wdpch.h"
#include "ImageImporter.h"
#include "stb_image.h"
#include "Waldem/Renderer/Texture.h"
#include <filesystem>
#include <fstream>
#include "..\Engine.h"

namespace Waldem
{
    Texture2D* CImageImporter::Import(const Path& path, bool relative)
    {
        Texture2D* result = nullptr;
        
        WString fileName = path.filename().string();
        
        int width, height, channels;
        unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
        size_t dataSize = width * height * channels;
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
            
            result = Renderer::CreateTexture(fileName, width, height, TextureFormat::R8G8B8A8_UNORM, rgbaData);
            stbi_image_free(data);

            if(channels == 3)
            {
                delete rgbaData;
            }
        }
        else
        {
		    WD_CORE_ERROR("Failed to load Image from path: {0}", path);
        }

        return result;
    }
}
