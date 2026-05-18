#pragma once
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Renderer/Animation/AnimationClip.h"

namespace Waldem
{
    struct AnimationClipReference : AssetReference
    {
        AnimationClip* Clip = nullptr;

        AnimationClipReference(Path reference = "Empty") : AssetReference(reference) {}
        AnimationClipReference(AnimationClip* clip) : Clip(clip) {}

        void LoadAsset() override
        {
            Clip = CContentManager::LoadAsset<AnimationClip>(Reference);
        }

        AssetType GetType() override { return AssetType::Animation; }

        bool IsValid() const { return Clip; }
    };
}
