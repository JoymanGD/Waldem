#include "wdpch.h"
#include "Model.h"
#include "Mesh.h"

namespace Waldem
{
    std::vector<Texture2D*> CModel::GetTextures()
    {
        std::vector<Texture2D*> textures;

        for (auto mesh : Meshes)
        {
            textures.push_back(mesh->CurrentMaterial->GetDiffuseTexture());
        }

        return textures;
    }
}
