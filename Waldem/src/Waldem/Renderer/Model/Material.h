#pragma once
#include "../Texture.h"
#include "Waldem/Renderer/Shader.h"

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
        
    private:
        Texture2D* DiffuseTexture;
    };
}
