#pragma once
#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    struct MeshComponent
    {
        MeshComponent(Mesh* mesh) : Mesh(mesh) {}
        Mesh* Mesh;
    };
}
