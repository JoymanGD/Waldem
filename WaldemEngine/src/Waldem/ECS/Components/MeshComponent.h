#pragma once
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Resources/ResourceManager.h"

namespace Waldem
{
    struct WALDEM_API MeshComponent
    {
        CMesh* Mesh = nullptr;
        uint DrawId = 0;

        MeshComponent() = default;
        MeshComponent(CMesh* mesh) : Mesh(mesh) {}
    };
}
