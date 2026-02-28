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
        case AssetType::Prefab:
            return curentFolder.append("Prefabs");
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

    inline WString AssetTypeToString(AssetType type)
    {
        switch (type)
        {
        case AssetType::Texture:   return "Texture";
        case AssetType::Mesh:      return "Mesh";
        case AssetType::Model:     return "Model";
        case AssetType::Material:  return "Material";
        case AssetType::Animation: return "Animation";
        case AssetType::Audio:     return "Audio";
        case AssetType::Scene:     return "Scene";
        case AssetType::Prefab:    return "Prefab";
        case AssetType::Script:    return "Script";
        case AssetType::Unknown:   return "Unknown";
        default:                   return "Invalid";
        }
    }

    inline WString AssetTypeToExtension(AssetType type)
    {
        switch (type)
        {
        case AssetType::Texture:   return ".img";
        case AssetType::Mesh:      return ".mesh";
        case AssetType::Model:     return ".model";
        case AssetType::Material:  return ".mat";
        case AssetType::Animation: return ".anim";
        case AssetType::Audio:     return ".sound";
        case AssetType::Scene:     return ".scene";
        case AssetType::Prefab:    return ".prefab";
        case AssetType::Script:    return ".script";
        case AssetType::Unknown:   return "Unknown";
        default:                   return "Invalid";
        }
    }

    inline AssetType ExtensionToAssetType(WString extension)
    {
        if(extension == ".img") return AssetType::Texture;
        if(extension == ".mesh") return AssetType::Mesh;
        if(extension == ".model") return AssetType::Model;
        if(extension == ".mat") return AssetType::Material;
        if(extension == ".anim") return AssetType::Animation;
        if(extension == ".sound") return AssetType::Audio;
        if(extension == ".scene") return AssetType::Scene;
        if(extension == ".prefab") return AssetType::Prefab;
        if(extension == ".script") return AssetType::Script;
        
        return AssetType::Unknown;
    }

    inline WString ExtensionToAssetString(WString extension)
    {
        if (extension == ".mesh") return "Mesh";
        if (extension == ".model") return "Model";
        if (extension == ".sound") return "Audio";
        if (extension == ".img") return "Texture";
        if (extension == ".anim") return "Animation";
        if (extension == ".mat") return "Material";
        if (extension == ".scene") return "Scene";
        if (extension == ".prefab") return "Prefab";
        if (extension == ".script") return "Script";
        
        return "Invalid";
    }

    inline bool IsSupportedExtension(WString extension)
    {
        return 
            extension == ".mesh" ||
            extension == ".model" ||
            extension == ".sound" ||
            extension == ".img" ||
            extension == ".anim" ||
            extension == ".mat" ||
            extension == ".scene" ||
            extension == ".prefab" ||
            extension == ".script";
    }
}
