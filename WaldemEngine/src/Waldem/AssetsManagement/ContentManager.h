#pragma once
#include "AudioImporter.h"
#include "ImageImporter.h"
#include "ModelImporter.h"
#include "Waldem/Events/FileEvent.h" 

namespace Waldem
{
    using FileDroppedEventHandler = std::function<void(Path)>;
    
    class WALDEM_API CContentManager
    {
    public:
        bool ImportTo(const Path& from, Path& to);
        
        template <class T>
        T* LoadAsset(const Path& inPath)
        {
            Asset* asset = nullptr;

            if(exists(inPath))
            {
                std::ifstream inFile(inPath.c_str(), std::ios::binary | std::ios::ate);
                if (inFile.is_open())
                {
                    std::streamsize size = inFile.tellg();
                    inFile.seekg(0, std::ios::beg); 

                    unsigned char* buffer = new unsigned char[size];
                    if (inFile.read((char*)buffer, size))
                    {
                        WDataBuffer inData = WDataBuffer(buffer, size);
                    
                        asset = new T();
                        asset->Type = ExtensionToAssetType(inPath.extension().string());
                        inData >> asset->Hash;
                        asset->Deserialize(inData);
                    }
                    else
                    {
                        WD_CORE_ERROR("Failed to read stream: {0}", inPath.string());
                    }
                
                    delete[] buffer;
                    inFile.close();
                }
                else
                {
                    WD_CORE_ERROR("Failed to open stream: {0}", inPath.string());
                }
            }
            else
            {
                WD_CORE_ERROR("Failed to load asset: {0}", inPath.string());
            }

            return (T*)asset;
        }
        
        template<typename T>
        bool LoadAsset(const Path& inPath, T& outAsset);

        bool Broadcast(Event& event);
        void SubscribeToFileDroppedEvent(FileDroppedEventHandler handler);
    private:
        CModelImporter ModelImporter;
        CImageImporter ImageImporter;
        CAudioImporter AudioImporter;
        WArray<FileDroppedEventHandler> FileDroppedEventHandlers;
        
        WArray<Asset*> ImportInternal(const Path& path);
    };
}
