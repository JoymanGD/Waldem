#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    struct MeshReference : AssetReference
    {
        CMesh* Mesh = nullptr;
        
        void LoadAsset() override
        {
            Mesh = CContentManager::LoadAsset<CMesh>(Reference);
        }

        AssetType GetType() override { return AssetType::Mesh; }

        bool IsValid() const { return Mesh; }
    };
}
