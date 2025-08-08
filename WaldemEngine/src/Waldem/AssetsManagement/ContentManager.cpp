#include "wdpch.h"
#include "ContentManager.h"

namespace Waldem
{
    WArray<Asset*> CContentManager::ImportInternal(const Path& path)
    {
        auto extension = path.extension().string();

        WArray<Asset*> assets;

        if (extension == ".png" || extension == ".jpg")
        {
            auto texture = ImageImporter.Import(path);
            assets.Add(&texture->Desc);
        }
        else if (extension == ".gltf" || extension == ".glb")
        {
            auto model = ModelImporter.Import(path);

            for (auto mesh : model->GetMeshes())
            {
                assets.Add(mesh);
            }
        }
        else if (extension == ".wav")
        {
            assets.Add(AudioImporter.Import(path));
        }

        return assets;
    }

    bool CContentManager::ImportTo(const Path& from, Path& to)
    {
        WArray<Asset*> assets = ImportInternal(from);
        bool multipleAssets = assets.Num() > 1;

        for (auto asset : assets)
        {
            WDataBuffer outData;
            asset->Serialize(outData);

            //add header
            uint64 hash = HashFromData(outData.GetData(), outData.GetSize());
            outData.Prepend(&hash, sizeof(uint64));

            auto extension = AssetTypeToExtension(asset->Type);
            Path toPath = to;
            toPath /= asset->Name.ToString();
            toPath.replace_extension(extension.ToString());

            if(multipleAssets)
            {
                auto folder = from.stem();
                toPath = toPath.parent_path() / folder / toPath.filename();
                create_directories(toPath.parent_path());
            }
            
            std::ofstream outFile(toPath.c_str(), std::ios::binary);
            outFile.write(static_cast<const char*>(outData.GetData()), outData.GetSize());
            outFile.close();

            if (!outFile)
            {
                WD_CORE_ERROR("Failed to import file from {0} to {1}", from.string(), toPath.string());
                return false;
            }
        }
        
        return true;
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

                    outAsset.Type = ExtensionToAssetType(inPath.extension().string());
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