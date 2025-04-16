#pragma once
#include "Waldem/Audio/AudioChannel.h"
#include "Waldem/Serialization/Serializable.h"

namespace Waldem
{
    class AudioClip : ISerializable
    {
    public:
        uint8* Data;
        uint Length;
        uint8 ChannelsNum; //1-mono, 2-stereo
        uint16 Stride;
        AudioChannel* CurrentChannel = nullptr;

        void Serialize(WDataBuffer& outData) override
        {
            outData << Length;
            outData << ChannelsNum;
            outData << Stride;
            outData.Write(Data, Length);
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> Length;
            inData >> ChannelsNum;
            inData >> Stride;
            inData.Read(Data, Length);
        }
    };
}
