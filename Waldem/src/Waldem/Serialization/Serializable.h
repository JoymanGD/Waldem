#pragma once
#include "Waldem/Types/DataBuffer.h"

namespace Waldem
{
    struct WALDEM_API ISerializable
    {
    public:
        virtual void Serialize(WDataBuffer& outData) = 0;
        virtual void Deserialize(WDataBuffer& inData) = 0;
    };
}