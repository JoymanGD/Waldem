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