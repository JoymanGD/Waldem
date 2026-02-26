#pragma once
#include "Waldem/Editor/AssetReference/MeshReference.h"
#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API MeshComponent
    {
        FIELD(Type=AssetReference)
        MeshReference MeshRef;
        
        DrawIndexedCommand DrawCommand;
        
        MeshComponent() {}

        bool IsValid() const { return MeshRef.IsValid(); }
    };
}
#include "MeshComponent.generated.h"
