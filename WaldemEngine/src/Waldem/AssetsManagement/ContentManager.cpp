#include "wdpch.h"
#include "ContentManager.h"

namespace Waldem
{
    Asset* CContentManager::ImportAssetInternal(const Path& path)
    {
        std::string extension = path.extension().string();

        Asset* asset = nullptr;

        if (extension == ".png" || extension == ".jpg")
        {
            auto texture = ImageImporter.Import(path);
            asset = (Asset*)&texture->Desc;
        }
        else if (extension == ".gltf" || extension == ".glb")
        {
            asset = (Asset*)ModelImporter.Import(path);
        }
        else if (extension == ".wav")
        {
            asset = (Asset*)AudioImporter.Import(path);
        }

        return asset;
    }
    
    bool CContentManager::ImportAsset(const Path& path)
    {
        Asset* asset = ImportAssetInternal(path);

        if(asset)
        {
            auto assetPath = GetPathForAsset(asset->Type);

            return ImportAssetTo(path, assetPath);
        }
        
        return false;
    }

    bool CContentManager::ImportAssetTo(const Path& inPath, Path& outPath)
    {
        Asset* asset = ImportAssetInternal(inPath);

        if(asset)
        {
            WDataBuffer outData;
            asset->Serialize(outData);

            //add header
            uint64 hash = HashFromData(outData.GetData(), outData.GetSize());
            outData.Prepend(&hash, sizeof(uint64));
            outData.Prepend(&asset->Type, sizeof(AssetType));

            outPath /= inPath.filename();
            outPath.replace_extension(".ass");
            
            std::ofstream outFile(outPath.c_str(), std::ios::binary);
            outFile.write(static_cast<const char*>(outData.GetData()), outData.GetSize());
            outFile.close();

            if (outFile)
            {
                return true;
            }
        }
        
        return false;
    }

    template<typename T>
    Asset* CContentManager::LoadAsset(const Path& inPath)
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
                    inData >> asset->Type;
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
    bool CContentManager::LoadAsset(const Path& inPath, T& outAsset)
    {
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
                    
                    inData >> outAsset.Type;
                    inData >> outAsset.Hash;
                    outAsset.Deserialize(inData);
                
                    delete[] buffer;
                    inFile.close();

                    return true;
                }
                
                delete[] buffer;
                inFile.close();
                
                return false;
            }
        }
        else
        {
            WD_CORE_ERROR("Failed to load asset: {0}", inPath.string());
        }

        return false;
    }

    bool CContentManager::Broadcast(Event& event)
    {
        bool handled = false;
                
        auto eventType = event.GetEventType();
                
        switch (eventType)
        {
        case EventType::FileDropped:
            {
                FileDroppedEvent& fileDroppedEvent = static_cast<FileDroppedEvent&>(event);
                for (auto& handler : FileDroppedEventHandlers)
                {
                    handler(fileDroppedEvent.GetPath());
                }

                handled = !FileDroppedEventHandlers.IsEmpty();
                        
                break;
            }
        default:
            break;
        }

        return handled;
    }

    void CContentManager::SubscribeToFileDroppedEvent(FileDroppedEventHandler handler)
    {
        FileDroppedEventHandlers.Add(handler);
    }
}