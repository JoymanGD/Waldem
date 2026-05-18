#pragma once
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/WArray.h"
#include "Waldem/Types/WMap.h"
#include "Waldem/Types/String.h"
#include "Waldem/Types/Types.h"

namespace Waldem
{
    struct PositionKey { float Time; Vector3 Value; };
    struct RotationKey { float Time; Quaternion Value; };
    struct ScaleKey    { float Time; Vector3 Value; };

    struct AnimationChannel
    {
        WString BoneName;
        WArray<PositionKey> PositionKeys;
        WArray<RotationKey> RotationKeys;
        WArray<ScaleKey>    ScaleKeys;

        void Serialize(WDataBuffer& outData);
        void Deserialize(WDataBuffer& inData);
    };

    class WALDEM_API AnimationClip : public Asset
    {
    public:
        AnimationClip() : Asset(AssetType::Animation) {}
        AnimationClip(WString name) : Asset(name, AssetType::Animation) {}

        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

        // Rebuild name->channel index after load (not serialized itself)
        void BuildLookup();

        // Returns -1 if not found
        int FindChannel(const WString& boneName) const;

        float DurationTicks = 0.0f;
        float TicksPerSecond = 25.0f;
        WArray<AnimationChannel> Channels;

    private:
        WMap<WString, int> ChannelLookup;
    };
}
