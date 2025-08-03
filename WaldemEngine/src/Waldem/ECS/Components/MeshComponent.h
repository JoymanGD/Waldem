#pragma once
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Resources/ResourceManager.h"

namespace Waldem
{
    struct WALDEM_API MeshComponent
    {
        COMPONENT(MeshComponent)
            FIELD(AssetReference, Mesh)
        END_COMPONENT()

        AssetReference Mesh;
        // CMesh* Mesh = nullptr;
        uint DrawId = 0;

        // MeshComponent(CMesh* mesh) : Mesh(mesh) {}
    };
}
