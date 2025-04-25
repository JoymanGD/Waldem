#pragma once
#include "Serializable.h"

namespace Waldem
{
    enum class AssetType
    {
        Texture,
        Mesh,
        Model,
        Material,
        Animation,
        Audio,
        Scene,
        Script,
        Unknown
    };
    
    class WALDEM_API Asset : public ISerializable
    {
    public:
        Asset(AssetType type = AssetType::Unknown) : Type(type) {}
        
        AssetType Type = AssetType::Unknown;
        uint64 Hash = 0;
    };
}
