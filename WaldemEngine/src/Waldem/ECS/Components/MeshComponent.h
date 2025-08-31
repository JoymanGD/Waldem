#pragma once
#include "Waldem/Editor/AssetReference/MeshReference.h"
#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    struct WALDEM_API MeshComponent
    {
        COMPONENT(MeshComponent)
            FIELD(AssetReference, Mesh)
        END_COMPONENT()

        MeshReference MeshRef;
        DrawCommand DrawCommand;

        bool IsValid() const { return MeshRef.IsValid(); }
    };
}
