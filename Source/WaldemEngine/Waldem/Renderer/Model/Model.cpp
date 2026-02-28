#include "wdpch.h"
#include "Model.h"

namespace Waldem
{
    void CModelNode::Serialize(WDataBuffer& outData)
    {
        Name.Serialize(outData);
        outData << ParentIndex;
        outData << LocalTransform;

        const uint meshCount = (uint)MeshPaths.Num();
        outData << meshCount;
        for (uint i = 0; i < meshCount; ++i)
        {
            outData << MeshPaths[i];
        }
    }

    void CModelNode::Deserialize(WDataBuffer& inData)
    {
        Name.Deserialize(inData);
        inData >> ParentIndex;
        inData >> LocalTransform;

        uint meshCount = 0;
        inData >> meshCount;
        MeshPaths.Resize(meshCount);
        for (uint i = 0; i < meshCount; ++i)
        {
            inData >> MeshPaths[i];
        }
    }

    void CModel::Serialize(WDataBuffer& outData)
    {
        const uint version = 1;
        outData << version;
        Nodes.Serialize(outData);
    }

    void CModel::Deserialize(WDataBuffer& inData)
    {
        uint version = 0;
        inData >> version;
        Nodes.Deserialize(inData);
    }
}
