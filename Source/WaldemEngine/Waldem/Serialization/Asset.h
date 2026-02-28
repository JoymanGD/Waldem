#pragma once
#include "Serializable.h"
#include "Waldem/Types/String.h"

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
        Prefab,
        Script,
        Unknown
    };
    
    class WALDEM_API Asset : public ISerializable
    {
public:
        Asset() {}
        Asset(AssetType type) : Type(type) {}
        Asset(WString name) : Name(name) {}
        Asset(WString name, AssetType type) : Name(name), Type(type) {}

        void Serialize(WDataBuffer& outData) override {}
        void Deserialize(WDataBuffer& inData) override {}
        
        WString Name = "UnknownAsset";
        uint64 Hash = 0;
        AssetType Type = AssetType::Unknown;
    };
}
