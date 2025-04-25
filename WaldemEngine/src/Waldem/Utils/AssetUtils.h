#pragma once
#include <filesystem>

#include "Waldem/Serialization/Asset.h"

namespace Waldem
{
    class CMesh;
    class TextureDesc;

    template <typename T>
    AssetType GetAssetType()
    {
        static_assert(sizeof(T) == 0, "Unsupported type for conversion to AssetType");
        return AssetType::Unknown;
    }

    template<>
    inline AssetType GetAssetType<CMesh>()
    {
        return AssetType::Mesh;
    }

    template<>
    inline AssetType GetAssetType<TextureDesc>()
    {
        return AssetType::Texture;
    }
    
    inline Path GetPathForAsset(AssetType type)
    {
        Path curentFolder = CONTENT_PATH;
        
        switch (type)
        {
        case AssetType::Texture:
            return curentFolder.append("Textures");
        case AssetType::Mesh:
            return curentFolder.append("Meshes");
        case AssetType::Model:
            return curentFolder.append("Models");
        case AssetType::Material:
            return curentFolder.append("Materials");
        case AssetType::Animation:
            return curentFolder.append("Animations");
        case AssetType::Audio:
            return curentFolder.append("Audio");
        case AssetType::Scene:
            return curentFolder.append("Scenes");
        case AssetType::Script:
            return curentFolder.append("Scripts");
        case AssetType::Unknown:
            break;
        default:
            WD_CORE_ERROR("Asset type is not supported yet");
            break;
        }
        
        return "";
    }
}
