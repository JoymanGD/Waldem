#pragma once

#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/MathTypes.h"
#include "Waldem/Types/String.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    struct WALDEM_API CModelNode : ISerializable
    {
        WString Name = "Node";
        int ParentIndex = -1;
        Matrix4 LocalTransform = Matrix4(1.0f);
        WArray<Path> MeshPaths;

        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;
    };

    class WALDEM_API CModel : public Asset
    {
    public:
        CModel() : Asset(AssetType::Model) {}
        CModel(WString name) : Asset(name, AssetType::Model) {}

        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

        WArray<CModelNode> Nodes;
    };
}
