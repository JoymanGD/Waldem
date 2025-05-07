#pragma once
#include "Waldem/Import/ImageImporter.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Renderer/RenderTarget.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/WMap.h"
#include "Waldem/Utils/AssetUtils.h"
#include "Waldem/Utils/FileUtils.h"
#include <fstream>

#include "Waldem/Utils/DataUtils.h"

namespace Waldem
{
    class WALDEM_API ResourceManager
    {
    public:
        ResourceManager() = default;
        
        static RenderTarget* CreateRenderTarget(WString name, int width, int height, TextureFormat format);
        static RenderTarget* ResizeRenderTarget(WString name, int width, int height);
        static Buffer* CreateBuffer(WString name, BufferType type, void* data, uint32_t size, uint32_t stride);
        static RenderTarget* CloneRenderTarget(RenderTarget* renderTarget);
        static Buffer* CloneBuffer(Buffer* buffer);
        static Texture2D* LoadTexture(const Path& path);
        static RenderTarget* GetRenderTarget(WString name) { return RenderTargets[name]; }
        static Texture2D* GetTexture(WString name) { return Textures[name]; }
        static Buffer* GetBuffer(WString name) { return Buffers[name]; }

        static uint64 ExportAsset(Asset* asset)
        {
            WDataBuffer dataBuffer;
            asset->Serialize(dataBuffer);

            uint64 hash = HashFromData(dataBuffer.GetData(), dataBuffer.GetSize());

            if(hash == asset->Hash)
            {
                return asset->Hash;
            }

            // // delete previous asset
            // if(asset->Hash)
            // {
            //     Path path = GetPathForAsset(asset->Type);
            //     path.append(std::to_string(asset->Hash));
            //     path.replace_extension(".ass");
            //
            //     if (exists(path))
            //     {
            //         std::filesystem::remove(path);
            //     }
            // }

            Path path = GetPathForAsset(asset->Type);
            path.append(std::to_string(hash));
            path.replace_extension(".ass");

            if (!exists(path))
            {
                create_directories(Path(path).parent_path());
                std::ofstream outFile(path.c_str(), std::ios::binary);
                outFile.write(static_cast<const char*>(dataBuffer.GetData()), dataBuffer.GetSize());
            }

            return hash;
        }
        
        template <typename T>
        static T* ImportAsset(uint64 hash)
        {
            Asset* asset = nullptr;
        
            AssetType type = GetAssetType<T>();
            Path path = GetPathForAsset(type);
            path.append(std::to_string(hash));
            path.replace_extension(".ass");

            if(exists(path))
            {
                std::ifstream inFile(path.c_str(), std::ios::binary | std::ios::ate);
                if (inFile.is_open())
                {
                    std::streamsize size = inFile.tellg();
                    inFile.seekg(0, std::ios::beg); 

                    unsigned char* buffer = new unsigned char[size];
                    if (inFile.read((char*)buffer, size))
                    {
                        asset = new T();
                        asset->Hash = hash;
                        WDataBuffer inData = WDataBuffer(buffer, size);
                        asset->Deserialize(inData);
                    }
                    delete[] buffer;
                    inFile.close();
                }
            }
            else
            {
                WD_CORE_ERROR("Failed to load asset: {0}", path.string());
            }

            return (T*)asset;
        }
        
        static void SerializeAsset(WDataBuffer& dataBuffer, Asset* asset)
        {
            if(asset)
            {
                dataBuffer << ExportAsset(asset);
                return;
            }

            dataBuffer << 0;
        }
        
        template <typename T>
        static void DeserializeAsset(WDataBuffer& dataBuffer, T*& asset)
        {
            uint64 hash;
            dataBuffer >> hash;
            
            if(hash > 0)
            {
                asset = ImportAsset<T>(hash);
            }
        }
        
    private:
        inline static WMap<WString, RenderTarget*> RenderTargets;
        inline static WMap<WString, Texture2D*> Textures;
        inline static WMap<WString, Buffer*> Buffers;

        inline static ImageImporter ImageImporter;
    };
}
