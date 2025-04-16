#pragma once
#include "Waldem/Audio/AudioClip.h"
#include "Waldem/Audio/Audio.h"
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API AudioSource : IComponent<AudioSource>
    {
        AudioClip* Clip = nullptr;
        float Range = 5.0f;
        float Volume = 1.0f;
        bool Loop = false;
        bool Spatial = true;

        AudioSource() = default;
        AudioSource(AudioClip* clip, float range, bool playOnStart, float volume = 1.0f, bool loop = false, bool spatial = true) : Clip(clip), Range(range), Volume(volume), Loop(loop), Spatial(spatial)
        {
            if(playOnStart)
            {
                Audio::Play(Clip, volume, loop);
            }
        }

        void Serialize(WDataBuffer& outData) override
        {
            // Clip->Serialize(outData);
            outData << Range;
            outData << Volume;
            outData << Loop;
            outData << Spatial;
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            // Clip->Deserialize(inData);
            inData >> Range;
            inData >> Volume;
            inData >> Loop;
            inData >> Spatial;
        }
    };
}
