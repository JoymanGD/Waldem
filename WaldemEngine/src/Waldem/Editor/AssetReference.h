#pragma once

namespace Waldem
{
    class CContentManager;

    struct AssetReference
    {
        Path Reference = "Empty";

        virtual void LoadAsset(CContentManager* contentManager) = 0;
        virtual AssetType GetType() = 0;

    protected:
        AssetType Type = AssetType::Unknown;
    };
}
