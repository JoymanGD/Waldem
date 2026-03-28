#pragma once

#include "Waldem/Serialization/Asset.h"

namespace Waldem
{
    class WALDEM_API ScriptAsset : public Asset
    {
    public:
        ScriptAsset()
        {
            Type = AssetType::Script;
        }

        void Serialize(WDataBuffer& outData) override
        {
            outData << AssemblyPath;
            outData << NamespaceName;
            outData << ClassName;
        }

        void Deserialize(WDataBuffer& inData) override
        {
            inData >> AssemblyPath;
            inData >> NamespaceName;
            inData >> ClassName;
        }

        Path AssemblyPath = "";
        WString NamespaceName = "";
        WString ClassName = "";
    };
}
