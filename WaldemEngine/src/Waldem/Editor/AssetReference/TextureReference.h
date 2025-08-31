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
            auto path = Reference;
            path.replace_extension(".img");
            auto textureDesc = CContentManager::LoadAsset<TextureDesc>(path);
            Texture = Renderer::CreateTexture2D(textureDesc->Name, textureDesc->Width, textureDesc->Height, textureDesc->Format, textureDesc->Data);
        }

        AssetType GetType() override { return AssetType::Texture; }

        bool IsValid() const { return Texture; }
    };
}
