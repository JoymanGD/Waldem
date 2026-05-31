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
            const MeshAssetKind meshKind = CContentManager::GetMeshAssetKind(Reference);
            if (meshKind == MeshAssetKind::Static)
            {
                WD_CORE_ERROR("Attempted to load static mesh {0} into SkeletalMeshComponent. Use MeshComponent instead.", Reference.string());
                Mesh = nullptr;
                return;
            }

            if (meshKind == MeshAssetKind::Unknown)
            {
                WD_CORE_ERROR("Failed to determine mesh type for {0}.", Reference.string());
                Mesh = nullptr;
                return;
            }

            Mesh = CContentManager::LoadAsset<SkeletalMesh>(Reference);
        }

        AssetType GetType() override { return AssetType::Mesh; }

        bool IsValid() const { return Mesh; }
    };
}
