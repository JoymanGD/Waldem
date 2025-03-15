#pragma once
#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    struct MeshComponent
    {
        MeshComponent(CMesh* mesh) : Mesh(mesh) {}
        CMesh* Mesh;
    };
}
