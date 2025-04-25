#pragma once
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Resources/ResourceManager.h"
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
        float Roughness = 0.0f;
    };
    
    class Material : public ISerializable
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
        
        void Serialize(WDataBuffer& outData) override
        {
            uint64 hash;
            hash = Diffuse ? ResourceManager::ExportAsset(&Diffuse->Desc) : 0;
            outData << hash;
            hash = Normal ? ResourceManager::ExportAsset(&Normal->Desc) : 0;
            outData << hash;
            hash = MetalRoughness ? ResourceManager::ExportAsset(&MetalRoughness->Desc) : 0;
            outData << hash;
            outData << Albedo;
            outData << Metallic;
            outData << Roughness;
        }

        Texture2D* DeserializeTexture(uint64 hash)
        {
            if(hash > 0)
            {
                auto textureDesc = ResourceManager::ImportAsset<TextureDesc>(hash);

                return Renderer::CreateTexture(*textureDesc);
            }

            return nullptr;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            uint64 hash;
            inData >> hash;
            Diffuse = DeserializeTexture(hash);
            inData >> hash;
            Normal = DeserializeTexture(hash);
            inData >> hash;
            MetalRoughness = DeserializeTexture(hash);
            inData >> Albedo;
            inData >> Metallic;
            inData >> Roughness;
        }

        Vector4 Albedo = Vector4(1.0f);
        float Metallic = 0.0f;
        float Roughness = 0.0f;
        
    private:
        Texture2D* Diffuse = nullptr;
        Texture2D* Normal = nullptr;
        Texture2D* MetalRoughness = nullptr;
    };
}
