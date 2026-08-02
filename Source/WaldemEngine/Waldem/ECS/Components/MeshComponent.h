#pragma once
#include "Waldem/AssetsManagement/AssetReference/MeshReference.h"
#include "Waldem/AssetsManagement/AssetReference/MaterialReference.h"
#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API MeshComponent
    {
        FIELD()
        MeshReference MeshRef;

        FIELD()
        MaterialReference MaterialRef;
        
        DrawIndexedCommand DrawCommand;
        
        MeshComponent() {}
        MeshComponent(MeshReference meshRef) : MeshRef(meshRef) {}
        MeshComponent(MeshReference meshRef, MaterialReference materialRef) : MeshRef(meshRef), MaterialRef(materialRef) {}

        bool IsValid() const { return MeshRef.IsValid(); }
    };
}
#include "MeshComponent.generated.h"
