#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Model/StaticMesh.h"

namespace Waldem
{
    struct MeshReference : AssetReference
    {
        StaticMesh* Mesh = nullptr;

        MeshReference(Path reference = "Empty") : AssetReference(reference) {}
        MeshReference(StaticMesh* mesh) : Mesh(mesh) {}
        
        void LoadAsset() override
        {
            Mesh = CContentManager::LoadAsset<StaticMesh>(Reference);
        }

        AssetType GetType() override { return AssetType::Mesh; }

        bool IsValid() const { return Mesh; }
    };
}
