#pragma once
#include "Waldem/Renderer/Texture.h"

namespace Waldem
{
    struct MaterialShaderAttribute
    {
        int DiffuseTextureID = -1;
        int NormalTextureID = -1;
        int ORMTextureID = -1;
        int ClearCoatTextureIndex = -1;

        Vector4 Albedo = Vector4(1.0f);
        float Metallic = 0.0f;
        float Roughness = 0.0f;
    };
    
    class Material
    {
    public:
        Material() = default;
        Material(Texture2D* diffuse, Texture2D* normal, Texture2D* metalRoughness) : Diffuse(diffuse), Normal(normal), MetalRoughness(metalRoughness) {}

        Texture2D* GetDiffuseTexture() { return Diffuse; }
        Texture2D* GetNormalTexture() { return Normal; }
        Texture2D* GetORMTexture() { return MetalRoughness; }

        bool HasDiffuseTexture() { return Diffuse != nullptr; }
        bool HasNormalTexture() { return Normal != nullptr; }
        bool HasORMTexture() { return MetalRoughness != nullptr; }

        Vector4 Albedo = Vector4(1.0f);
        float Metallic = 0.0f;
        float Roughness = 0.0f;
        
    private:
        Texture2D* Diffuse;
        Texture2D* Normal;
        Texture2D* MetalRoughness;
    };
}
