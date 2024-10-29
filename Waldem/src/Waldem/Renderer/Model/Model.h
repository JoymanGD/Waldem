#pragma once
#include "Mesh.h"

namespace Waldem
{
    class Model
    {
    private:
        std::vector<Mesh*> Meshes;
    public:
        Model() = default;
        Model(std::vector<Mesh*> meshes) : Meshes(meshes) {}

        std::vector<Mesh*> GetMeshes() { return Meshes; }

        void AddMesh(Mesh* mesh) { Meshes.push_back(mesh); }
    };
}
