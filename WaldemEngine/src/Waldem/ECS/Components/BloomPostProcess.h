#pragma once
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API BloomPostProcess : IComponent<BloomPostProcess>
    {
        float BrightThreshold = 0.8f;
        float BloomIntensity = 2.f;
        Vector2 TexelSize = Vector2(0.001f);

        BloomPostProcess(float brightThreshold = 0.8f, float bloomIntensity = 2.f, Vector2 texelSize = Vector2(0.001f))
            : BrightThreshold(brightThreshold), BloomIntensity(bloomIntensity), TexelSize(texelSize) {}

        void Serialize(WDataBuffer& outData) override
        {
            outData << BrightThreshold;
            outData << BloomIntensity;
            outData << TexelSize;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            inData >> BrightThreshold;
            inData >> BloomIntensity;
            inData >> TexelSize;
        }
    };
}
