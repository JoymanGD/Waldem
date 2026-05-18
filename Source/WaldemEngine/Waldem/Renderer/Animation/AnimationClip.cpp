#include "wdpch.h"
#include "AnimationClip.h"

namespace Waldem
{
    void AnimationChannel::Serialize(WDataBuffer& outData)
    {
        BoneName.Serialize(outData);
        uint posCount = (uint)PositionKeys.Num();
        uint rotCount = (uint)RotationKeys.Num();
        uint scaleCount = (uint)ScaleKeys.Num();
        outData << posCount;
        outData.Write(PositionKeys.GetData(), posCount * sizeof(PositionKey));
        outData << rotCount;
        outData.Write(RotationKeys.GetData(), rotCount * sizeof(RotationKey));
        outData << scaleCount;
        outData.Write(ScaleKeys.GetData(), scaleCount * sizeof(ScaleKey));
    }

    void AnimationChannel::Deserialize(WDataBuffer& inData)
    {
        BoneName.Deserialize(inData);
        uint posCount = 0;   inData >> posCount;
        PositionKeys.Resize(posCount);
        inData.Read(PositionKeys.GetData(), posCount * sizeof(PositionKey));
        uint rotCount = 0;   inData >> rotCount;
        RotationKeys.Resize(rotCount);
        inData.Read(RotationKeys.GetData(), rotCount * sizeof(RotationKey));
        uint scaleCount = 0; inData >> scaleCount;
        ScaleKeys.Resize(scaleCount);
        inData.Read(ScaleKeys.GetData(), scaleCount * sizeof(ScaleKey));
    }

    void AnimationClip::Serialize(WDataBuffer& outData)
    {
        outData << DurationTicks;
        outData << TicksPerSecond;
        uint channelCount = (uint)Channels.Num();
        outData << channelCount;
        for (uint i = 0; i < channelCount; ++i)
            Channels[i].Serialize(outData);
    }

    void AnimationClip::Deserialize(WDataBuffer& inData)
    {
        inData >> DurationTicks;
        inData >> TicksPerSecond;
        uint channelCount = 0;
        inData >> channelCount;
        Channels.Resize(channelCount);
        for (uint i = 0; i < channelCount; ++i)
            Channels[i].Deserialize(inData);
        BuildLookup();
    }

    void AnimationClip::BuildLookup()
    {
        for (uint i = 0; i < (uint)Channels.Num(); ++i)
            ChannelLookup[Channels[i].BoneName] = (int)i;
    }

    int AnimationClip::FindChannel(const WString& boneName) const
    {
        const int* idx = ChannelLookup.Find(boneName);
        return idx ? *idx : -1;
    }
}
