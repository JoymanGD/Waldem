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
            const MeshAssetKind meshKind = CContentManager::GetMeshAssetKind(Reference);
            if (meshKind == MeshAssetKind::Skeletal)
            {
                WD_CORE_ERROR("Attempted to load skeletal mesh {0} into MeshComponent. Use SkeletalMeshComponent instead.", Reference.string());
                Mesh = nullptr;
                return;
            }

            if (meshKind == MeshAssetKind::Unknown)
            {
                WD_CORE_ERROR("Failed to determine mesh type for {0}.", Reference.string());
                Mesh = nullptr;
                return;
            }

            Mesh = CContentManager::LoadAsset<StaticMesh>(Reference);
        }

        AssetType GetType() override { return AssetType::Mesh; }

        bool IsValid() const { return Mesh; }
    };
}
