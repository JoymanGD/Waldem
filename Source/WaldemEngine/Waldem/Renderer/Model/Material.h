#pragma once
#include "Waldem/Editor/AssetReference/TextureReference.h"
#include "Waldem/Types/DataBuffer.h"

namespace Waldem
{
    class Texture2D;

    struct MaterialShaderAttribute
    {
        int DiffuseTextureID = -1;
        int NormalTextureID = -1;
        int ORMTextureID = -1;
        int ClearCoatTextureIndex = -1;

        Vector4 Albedo = Vector4(1.0f);
        float Metallic = 0.0f;
        float Roughness = 1.0f;
    };
    
    class WALDEM_API Material : public Asset
    {
    public:
        Material() = default;
        Material(WString name, TextureReference diffuse, TextureReference normal, TextureReference metalRoughness) : Asset(name, AssetType::Material), DiffuseRef(diffuse), NormalRef(normal), MetalRoughnessRef(metalRoughness) {}
        Material(WString name, TextureReference diffuse, TextureReference normal, TextureReference metalRoughness, Vector4 albedo, float roughness, float metallic) : Asset(name, AssetType::Material), Albedo(albedo), Metallic(metallic), Roughness(roughness), DiffuseRef(diffuse), NormalRef(normal), MetalRoughnessRef(metalRoughness) { Type = AssetType::Material; }

        Texture2D* GetDiffuseTexture() { return DiffuseRef.Texture; }
        Texture2D* GetNormalTexture() { return NormalRef.Texture; }
        Texture2D* GetORMTexture() { return MetalRoughnessRef.Texture; }

        bool HasDiffuseTexture() { return DiffuseRef.IsValid(); }
        bool HasNormalTexture() { return NormalRef.IsValid(); }
        bool HasORMTexture() { return MetalRoughnessRef.IsValid(); }

        Path& GetDiffuseReference() { return DiffuseRef.Reference; }
        Path& GetNormalReference() { return NormalRef.Reference; }
        Path& GetORMReference() { return MetalRoughnessRef.Reference; }
        
        void Serialize(WDataBuffer& outData) override;
        
        void Deserialize(WDataBuffer& inData) override;

        Vector4 Albedo = Vector4(1.0f);
        float Metallic = 0.0f;
        float Roughness = 1.0f;
        
    private:
        TextureReference DiffuseRef = {};
        TextureReference NormalRef = {};
        TextureReference MetalRoughnessRef = {};
    };
}
