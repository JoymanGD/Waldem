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
        Material(Texture2D* diffuse, Texture2D* normal, Texture2D* metalRoughness, Vector4 albedo, float roughness, float metallic) : Diffuse(diffuse), Normal(normal), MetalRoughness(metalRoughness), Albedo(albedo), Roughness(roughness), Metallic(metallic) {}

        Texture2D* GetDiffuseTexture() { return Diffuse; }
        Texture2D* GetNormalTexture() { return Normal; }
        Texture2D* GetORMTexture() { return MetalRoughness; }

        bool HasDiffuseTexture() { return Diffuse != nullptr; }
        bool HasNormalTexture() { return Normal != nullptr; }
        bool HasORMTexture() { return MetalRoughness != nullptr; }

        void SerializeTexture(WDataBuffer& outData, Texture2D* texture)
        {
            if(texture)
            {
                uint64 hash = ResourceManager::ExportAsset(&texture->Desc);
                outData << hash;
                return;
            }

            outData << (uint64)0;
        }

        void DeserializeTexture(WDataBuffer& inData, Texture2D*& texture)
        {
            uint64 hash;
            inData >> hash;
            
            if(hash > 0)
            {
                auto textureDesc = ResourceManager::ImportAsset<TextureDesc>(hash);

                texture = Renderer::CreateTexture(textureDesc->Name, textureDesc->Width, textureDesc->Height, textureDesc->Format, textureDesc->Data);
            }
        }
        
        void Serialize(WDataBuffer& outData) override
        {
            SerializeTexture(outData, Diffuse);
            SerializeTexture(outData, Normal);
            SerializeTexture(outData, MetalRoughness);
            outData << Albedo;
            outData << Metallic;
            outData << Roughness;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            DeserializeTexture(inData, Diffuse);
            DeserializeTexture(inData, Normal);
            DeserializeTexture(inData, MetalRoughness);
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
