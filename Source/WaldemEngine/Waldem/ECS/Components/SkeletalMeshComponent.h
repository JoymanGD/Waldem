#pragma once
#include "Waldem/AssetsManagement/AssetReference/SkeletalMeshReference.h"
#include "Waldem/AssetsManagement/AssetReference/MaterialReference.h"
#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API SkeletalMeshComponent
    {
        FIELD()
        SkeletalMeshReference MeshRef;

        FIELD()
        MaterialReference MaterialRef;

        DrawIndexedCommand DrawCommand;
        Buffer* BoneMatricesBuffer = nullptr;
        int BoneCount = 0;

        SkeletalMeshComponent() {}
        SkeletalMeshComponent(SkeletalMeshReference meshRef) : MeshRef(meshRef) {}
        SkeletalMeshComponent(SkeletalMeshReference meshRef, MaterialReference materialRef) : MeshRef(meshRef), MaterialRef(materialRef) {}

        bool IsValid() const { return MeshRef.IsValid(); }
    };
}
#include "SkeletalMeshComponent.generated.h"
