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
        bool ImportAsset(const Path& path);
        bool ImportAssetTo(const Path& inPath, Path& outPath);

        bool Broadcast(Event& event);
        void SubscribeToFileDroppedEvent(FileDroppedEventHandler handler);
    private:
        CModelImporter ModelImporter;
        CImageImporter ImageImporter;
        CAudioImporter AudioImporter;
        WArray<FileDroppedEventHandler> FileDroppedEventHandlers;
        
        Asset* ImportAssetInternal(const Path& path);
    };
}
