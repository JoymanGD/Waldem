#include "wdpch.h"
#include "ContentManager.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Utils/AssetUtils.h"
#include "Waldem/Utils/DataUtils.h"

namespace Waldem
{
    WArray<Asset*> CContentManager::ImportInternal(const Path& from, Path& to, const ModelImportSettings* modelImportSettings)
    {
        auto extension = from.extension().string();

        WArray<Asset*> assets;

        IImporter* importer = nullptr;

        if (extension == ".png" || extension == ".jpg")
        {
            importer = &ImageImporter;
        }
        else if (extension == ".gltf" || extension == ".glb" || extension == ".fbx")
        {
            importer = &ModelImporter;
            if(modelImportSettings != nullptr)
            {
                ModelImporter.SetImportSettings(*modelImportSettings);
            }
        }
        else if (extension == ".wav")
        {
            importer = &AudioImporter;
        }

        if(importer == nullptr)
        {
            WD_CORE_ERROR("No importer registered for file extension: {0}", extension);
            return assets;
        }

        assets.AddRange(importer->ImportTo(from, to));
        if(importer == &ModelImporter)
        {
            ModelImporter.ResetImportSettings();
        }

        importer = nullptr;

        return assets;
    }

    bool CContentManager::ImportTo(const Path& from, Path& to)
    {
        ImportInProgress.store(true);
        ImportProgress.store(0.0f);
        {
            std::lock_guard<std::mutex> lock(ImportLabelMutex);
            ImportLabel = from.filename().string();
        }

        auto resetImportState = [&]()
        {
            ImportProgress.store(0.0f);
            ImportInProgress.store(false);
            std::lock_guard<std::mutex> lock(ImportLabelMutex);
            ImportLabel.clear();
        };

        WArray<Asset*> assets = ImportInternal(from, to, nullptr);
        bool multipleAssets = assets.Num() > 1;

        if(assets.IsEmpty())
        {
            resetImportState();
            return false;
        }

        const float saveStageStart = 0.2f;
        for (int assetIndex = 0; assetIndex < assets.Num(); ++assetIndex)
        {
            auto asset = assets[assetIndex];
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
                resetImportState();
                return false;
            }

            const float assetProgress = (float)(assetIndex + 1) / (float)assets.Num();
            ImportProgress.store(saveStageStart + (1.0f - saveStageStart) * assetProgress);
        }

        resetImportState();
        return true;
    }

    bool CContentManager::ImportTo(const Path& from, Path& to, const ModelImportSettings& modelImportSettings)
    {
        ImportInProgress.store(true);
        ImportProgress.store(0.0f);
        {
            std::lock_guard<std::mutex> lock(ImportLabelMutex);
            ImportLabel = from.filename().string();
        }

        auto resetImportState = [&]()
        {
            ImportProgress.store(0.0f);
            ImportInProgress.store(false);
            std::lock_guard<std::mutex> lock(ImportLabelMutex);
            ImportLabel.clear();
        };

        WArray<Asset*> assets = ImportInternal(from, to, &modelImportSettings);
        bool multipleAssets = assets.Num() > 1;

        if(assets.IsEmpty())
        {
            resetImportState();
            return false;
        }

        const float saveStageStart = 0.2f;
        for (int assetIndex = 0; assetIndex < assets.Num(); ++assetIndex)
        {
            auto asset = assets[assetIndex];
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
                resetImportState();
                return false;
            }

            const float assetProgress = (float)(assetIndex + 1) / (float)assets.Num();
            ImportProgress.store(saveStageStart + (1.0f - saveStageStart) * assetProgress);
        }

        resetImportState();
        return true;
    }

    bool CContentManager::GetImportStatus(float& outProgress, std::string& outLabel)
    {
        const bool inProgress = ImportInProgress.load();
        outProgress = ImportProgress.load();

        std::lock_guard<std::mutex> lock(ImportLabelMutex);
        outLabel = ImportLabel;
        return inProgress;
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
