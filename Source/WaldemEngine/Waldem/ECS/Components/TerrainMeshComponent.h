#pragma once
#include "Waldem/AssetsManagement/AssetReference/MeshReference.h"
#include "Waldem/AssetsManagement/AssetReference/MaterialReference.h"
#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    COMPONENT(Hidden)
    struct WALDEM_API TerrainMeshComponent
    {
        FIELD()
        MeshReference MeshRef;

        FIELD()
        MaterialReference MaterialRef;
        
        TerrainMeshComponent() {}
        TerrainMeshComponent(MeshReference meshRef) : MeshRef(meshRef) {}
        TerrainMeshComponent(MeshReference meshRef, MaterialReference materialRef) : MeshRef(meshRef), MaterialRef(materialRef) {}

        bool IsValid() const { return MeshRef.IsValid(); }
    };
}
#include "TerrainMeshComponent.generated.h"
