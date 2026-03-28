#pragma once

#include "Waldem/Editor/AssetReference.h"

namespace Waldem
{
    struct ScriptReference : AssetReference
    {
        ScriptReference(Path reference = "Empty") : AssetReference(reference) {}

        void LoadAsset() override
        {
            // Raw C# script references are resolved directly by the script runtime.
        }

        AssetType GetType() override { return AssetType::Script; }

        bool IsValid() const
        {
            return !Reference.empty() && Reference != "Empty";
        }
    };
}
