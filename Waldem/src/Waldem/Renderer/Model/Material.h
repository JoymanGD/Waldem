#pragma once
#include "Waldem/Renderer/Texture.h"

namespace Waldem
{
    class Material
    {
    public:
        Material() = default;
        Material(Texture2D* diffuseTexture)
        {
            DiffuseTexture = diffuseTexture;
        }

        Texture2D* GetDiffuseTexture() { return DiffuseTexture; }
        
    private:
        Texture2D* DiffuseTexture;
    };
}
