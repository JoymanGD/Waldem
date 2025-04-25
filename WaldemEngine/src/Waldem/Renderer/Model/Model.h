#pragma once
#include "Mesh.h"

namespace Waldem
{
    struct WALDEM_API CModel
    {
        CModel() = default;
        CModel(std::vector<CMesh*> meshes) : Meshes(meshes) {}

        std::vector<CMesh*> GetMeshes() { return Meshes; }
        void AddMesh(CMesh* mesh) { Meshes.push_back(mesh); }

        std::vector<Texture2D*> GetTextures()
        {
            std::vector<Texture2D*> textures;

            for (auto mesh : Meshes)
            {
                textures.push_back(mesh->CurrentMaterial->GetDiffuseTexture());
            }

            return textures;
        }
    private:
        std::vector<CMesh*> Meshes;
    };
}
