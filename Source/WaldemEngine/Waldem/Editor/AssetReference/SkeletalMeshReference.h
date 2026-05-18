#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Model/SkeletalMesh.h"

namespace Waldem
{
    struct SkeletalMeshReference : AssetReference
    {
        SkeletalMesh* Mesh = nullptr;

        SkeletalMeshReference(Path reference = "Empty") : AssetReference(reference) {}
        SkeletalMeshReference(SkeletalMesh* mesh) : Mesh(mesh) {}

        void LoadAsset() override
        {
            Mesh = CContentManager::LoadAsset<SkeletalMesh>(Reference);
        }

        AssetType GetType() override { return AssetType::Mesh; }

        bool IsValid() const { return Mesh; }
    };
}
