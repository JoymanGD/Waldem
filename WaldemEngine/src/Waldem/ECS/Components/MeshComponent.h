#pragma once
#include "Waldem/Editor/AssetReference/MeshReference.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Resources/ResourceManager.h"

namespace Waldem
{
    struct WALDEM_API MeshComponent
    {
        COMPONENT(MeshComponent)
            FIELD(AssetReference, Mesh)
        END_COMPONENT()

        MeshReference MeshRef;
        uint DrawId = -1;
        uint RTXInstanceId = -1;

        bool IsValid() const { return MeshRef.IsValid(); }
    };
}
