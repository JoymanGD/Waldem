#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    struct TextureReference : AssetReference
    {
        TextureReference(Path reference = "Empty") : AssetReference(reference) {}
        
        Texture2D* Texture = nullptr;
        
        void LoadAsset() override
        {
            if (Texture)
            {
                Renderer::Destroy(Texture);
                Texture = nullptr;
            }

            if (Reference.empty() || Reference == "Empty")
            {
                Texture = nullptr;
                return;
            }

            auto path = Reference;
            path.replace_extension(".img");
            auto textureDesc = CContentManager::LoadAsset<TextureDesc>(path);
            if (!textureDesc)
            {
                Texture = nullptr;
                return;
            }

            Texture = Renderer::CreateTexture2D(textureDesc->Name, textureDesc->Width, textureDesc->Height, textureDesc->Format, textureDesc->Data);
            delete textureDesc;
        }

        AssetType GetType() override { return AssetType::Texture; }

        bool IsValid() const { return Texture; }
    };
}
