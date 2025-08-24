#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Model/Material.h"

namespace Waldem
{
    struct MaterialReference : AssetReference
    {
        MaterialReference(Path reference = "Empty") : AssetReference(reference) {}
        
        Material* Mat = nullptr;
        
        void LoadAsset() override
        {
            auto path = Reference;
            path.replace_extension(".mat");
            Mat = CContentManager::LoadAsset<Material>(path);
        }

        AssetType GetType() override { return AssetType::Material; }

        bool IsValid() const { return Mat; }
    };
}
