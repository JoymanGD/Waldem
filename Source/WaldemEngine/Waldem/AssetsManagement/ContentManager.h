#pragma once
#include "AudioImporter.h"
#include "ImageImporter.h"
#include "ModelImporter.h"
#include "Waldem/Events/FileEvent.h" 
#include "Waldem/Utils/AssetUtils.h"
#include <fstream>
#include <atomic>
#include <mutex>
#include <string>

#include "Waldem/ProjectManagement/ProjectManager.h"

namespace Waldem
{
    using FileDroppedEventHandler = std::function<void(Path)>;

    enum class MeshAssetKind
    {
        Unknown = 0,
        Static,
        Skeletal
    };
    
    class WALDEM_API CContentManager
    {
    public:
        static bool ImportTo(const Path& from, Path& to);
        static bool ImportTo(const Path& from, Path& to, const ModelImportSettings& modelImportSettings);
        static bool GetImportStatus(float& outProgress, std::string& outLabel);
        static MeshAssetKind GetMeshAssetKind(const Path& inPath);

        template <class T>
        static T* LoadAsset(const Path& inPath)
        {
            Asset* asset = nullptr;
            Path finalPath = inPath;

            if (finalPath.is_relative())
            {
                finalPath = Path(PROJECT_CONTENT_PATH) / finalPath;
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
                        try
                        {
                            inData >> asset->Hash;
                            asset->Deserialize(inData);
                        }
                        catch (const std::exception& e)
                        {
                            WD_CORE_ERROR("Failed to deserialize asset {0}: {1}", finalPath.string(), e.what());
                            delete asset;
                            asset = nullptr;
                        }
                        catch (...)
                        {
                            WD_CORE_ERROR("Failed to deserialize asset {0}: unknown error", finalPath.string());
                            delete asset;
                            asset = nullptr;
                        }
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
        inline static std::atomic<bool> ImportInProgress = false;
        inline static std::atomic<float> ImportProgress = 0.0f;
        inline static std::mutex ImportLabelMutex;
        inline static std::string ImportLabel;
        
        static WArray<Asset*> ImportInternal(const Path& from, Path& to, const ModelImportSettings* modelImportSettings = nullptr);
    };
}
