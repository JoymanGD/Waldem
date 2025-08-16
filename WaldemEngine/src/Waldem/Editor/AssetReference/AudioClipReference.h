#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    struct AudioClipReference : AssetReference
    {
        AudioClip* Clip = nullptr;
        
        void LoadAsset(CContentManager* contentManager) override
        {
            Clip = contentManager->LoadAsset<AudioClip>(Reference);
        }

        AssetType GetType() override { return AssetType::Audio; }

        bool IsValid() const { return Clip; }
    };
}
