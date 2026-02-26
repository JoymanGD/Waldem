#pragma once

namespace Waldem
{
    class CContentManager;

    struct AssetReference
    {
        AssetReference(Path reference = "Empty") : Reference(reference) {}
        
        Path Reference = "Empty";

        virtual void LoadAsset() = 0;
        virtual AssetType GetType() = 0;
    };
}
