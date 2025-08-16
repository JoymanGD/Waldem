#pragma once
#include "Waldem/Audio/AudioChannel.h"
#include "Waldem/Serialization/Asset.h"

namespace Waldem
{
    class WALDEM_API AudioClip : public Asset
    {
    public:
        uint8* Data = nullptr;
        uint Length = 0;
        uint8 ChannelsNum = 2; //1-mono, 2-stereo
        uint16 Stride = 0;
        AudioChannel* CurrentChannel = nullptr;

        AudioClip()
        {
            Type = AssetType::Audio;
        }
        
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
            Data = new uint8[Length];
            inData.Read(Data, Length);
        }
    };
}
