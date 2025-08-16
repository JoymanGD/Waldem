#pragma once
#include "Waldem/Editor/AssetReference/MeshReference.h"
#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    struct WALDEM_API MeshComponent
    {
        COMPONENT(MeshComponent)
            FIELD(AssetReference, Mesh)
        END_COMPONENT()

        MeshReference MeshRef;

        bool IsValid() const { return MeshRef.IsValid(); }
    };
}
