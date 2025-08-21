#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"

namespace Waldem
{
    struct TextureReference : AssetReference
    {
        Texture2D* Texture = nullptr;
        
        void LoadAsset(CContentManager* contentManager) override
        {
            auto textureDesc = contentManager->LoadAsset<TextureDesc>(Reference);
            Texture = Renderer::CreateTexture(textureDesc->Name, textureDesc->Width, textureDesc->Height, textureDesc->Format, textureDesc->Data);
        }

        AssetType GetType() override { return AssetType::Texture; }

        bool IsValid() const { return Texture; }
    };
}
