#pragma once
#include "Mesh.h"

namespace Waldem
{
    struct WALDEM_API Model
    {
        Model() = default;
        Model(std::vector<Mesh*> meshes) : Meshes(meshes) {}

        std::vector<Mesh*> GetMeshes() { return Meshes; }
        void AddMesh(Mesh* mesh) { Meshes.push_back(mesh); }

        std::vector<Texture2D*> GetTextures()
        {
            std::vector<Texture2D*> textures;

            for (auto mesh : Meshes)
            {
                textures.push_back(mesh->MeshMaterial.GetDiffuseTexture());
            }

            return textures;
        }
    private:
        std::vector<Mesh*> Meshes;
    };
}
