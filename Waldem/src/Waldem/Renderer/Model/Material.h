#pragma once
#include "Waldem/Renderer/Texture.h"

namespace Waldem
{
    class Material
    {
    public:
        Material() = default;
        Material(Texture2D* diffuse, Texture2D* normal, Texture2D* metalRoughness) : Diffuse(diffuse), Normal(normal), MetalRoughness(metalRoughness) {}

        Texture2D* GetDiffuseTexture() { return Diffuse; }
        Texture2D* GetNormalTexture() { return Normal; }
        Texture2D* GetORMTexture() { return MetalRoughness; }
        
    private:
        Texture2D* Diffuse;
        Texture2D* Normal;
        Texture2D* MetalRoughness;
    };
}
