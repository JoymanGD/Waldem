#pragma once
#include "AudioImporter.h"
#include "ImageImporter.h"
#include "ModelImporter.h"
#include "Waldem/Events/FileEvent.h" 
#include "Waldem/Utils/AssetUtils.h"
#include <fstream>

namespace Waldem
{
    using FileDroppedEventHandler = std::function<void(Path)>;
    
    class WALDEM_API CContentManager
    {
    public:
        static bool ImportTo(const Path& from, Path& to);

        template <class T>
        static T* LoadAsset(const Path& inPath)
        {
            Asset* asset = nullptr;
            Path finalPath = inPath;

            if (finalPath.is_relative())
            {
                finalPath = Path(CONTENT_PATH) / finalPath;
            }

            if(exists(finalPath))
            {
                std::ifstream inFile(finalPath.c_str(), std::ios::binary | std::ios::ate);
                if (inFile.is_open())
                {
                    std::streamsize size = inFile.tellg();
                    inFile.seekg(0, std::ios::beg); 

                    unsigned char* buffer = new unsigned char[size];
                    if (inFile.read((char*)buffer, size))
                    {
                        WDataBuffer inData = WDataBuffer(buffer, size);
                    
                        asset = new T();
                        asset->Type = ExtensionToAssetType(finalPath.extension().string());
                        inData >> asset->Hash;
                        asset->Deserialize(inData);
                    }
                    else
                    {
                        WD_CORE_ERROR("Failed to read stream: {0}", finalPath.string());
                    }
                
                    delete[] buffer;
                    inFile.close();
                }
                else
                {
                    WD_CORE_ERROR("Failed to open stream: {0}", finalPath.string());
                }
            }
            else
            {
                WD_CORE_ERROR("Failed to load asset: {0}", finalPath.string());
            }

            return (T*)asset;
        }
        
        template<typename T>
        static bool LoadAsset(const Path& inPath, T& outAsset);

        static bool Broadcast(Event& event);
        static void SubscribeToFileDroppedEvent(FileDroppedEventHandler handler);
    private:
        inline static CModelImporter ModelImporter;
        inline static CImageImporter ImageImporter;
        inline static CAudioImporter AudioImporter;
        inline static WArray<FileDroppedEventHandler> FileDroppedEventHandlers;
        
        static WArray<Asset*> ImportInternal(const Path& from, Path& to);
    };
}
