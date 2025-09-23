#include "wdpch.h"
#include "ImageUtils.h"
#include <stb_image.h>
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Types/String.h"
#include "Waldem/Renderer/Texture.h"

namespace Waldem
{
    bool ImageUtils::TryLoadTexture(const Path& path, Texture2D*& outTexture, bool relative)
    {
        outTexture = LoadTexture(path);
        return outTexture != nullptr;
    }

    Texture2D* ImageUtils::LoadTexture(const Path& path)
    {
        Texture2D* outTexture = nullptr;
        TextureDesc* outTextureDesc = nullptr;

        if(TryLoadTextureDesc(path, outTextureDesc))
        {
            outTexture = Renderer::CreateTexture2D(outTextureDesc->Name, outTextureDesc->Width, outTextureDesc->Height, outTextureDesc->Format, outTextureDesc->Data);
            delete outTextureDesc;
        }
        
        return outTexture;
    }

    bool ImageUtils::TryLoadTextureDesc(const Path& path, TextureDesc*& outTexture, bool relative)
    {
        WString fileName = path.filename().string();
        
        int width, height, channels;
        
        Path finalPath = path;
        
        if(finalPath.is_relative())
        {
            finalPath = Path(CONTENT_PATH) / finalPath;
        }

        if(!finalPath.has_extension())
        {
            if(std::filesystem::exists(finalPath.string() + ".png"))
            {
                finalPath.replace_extension(".png");
            }
            else if(std::filesystem::exists(finalPath.string() + ".jpg"))
            {
                finalPath.replace_extension(".jpg");
            }
            else
            {
                WD_CORE_ERROR("Failed to load Image from path: {0}", path);
                return false;
            }
        }
        
        unsigned char *data = stbi_load(finalPath.string().c_str(), &width, &height, &channels, 0);
        if(data)
        {
            unsigned char* rgbaData = new unsigned char[width * height * 4];
            
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
            else if(channels == 4)
            {
                memcpy(rgbaData, data, width * height * 4);
            }
            else if (channels == 1)
            {
                for (int i = 0; i < width * height; ++i)
                {
                    rgbaData[i * 4 + 0] = data[i];
                    rgbaData[i * 4 + 1] = data[i];
                    rgbaData[i * 4 + 2] = data[i];
                    rgbaData[i * 4 + 3] = 255;
                }
            }
            
            outTexture = new TextureDesc(fileName, width, height, 1, TextureFormat::R8G8B8A8_UNORM, rgbaData);
            stbi_image_free(data);

            return true;
        }
        else
        {
            WD_CORE_ERROR("Failed to load Image from path: {0}", path);
            return false;
        }
    }
}
