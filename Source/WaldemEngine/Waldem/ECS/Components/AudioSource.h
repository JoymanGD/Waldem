#pragma once
#include "Waldem/Audio/Audio.h"
#include "ComponentBase.h"
#include "Waldem/Editor/AssetReference/AudioClipReference.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API AudioSource
    {
        FIELD(Type=AssetReference)
        AudioClipReference ClipRef;
        FIELD()
        float Range = 5.0f;
        FIELD()
        float Volume = 1.0f;
        FIELD()
        bool Loop = false;
        FIELD()
        bool Spatial = true;
        FIELD()
        bool PlayOnStart = false;
        
        AudioSource() {}

        AudioSource(float range, bool playOnStart, float volume = 1.0f, bool loop = false, bool spatial = true) : Range(range), Volume(volume), Loop(loop), Spatial(spatial)
        {
            PlayOnStart = playOnStart;
            
            if(PlayOnStart)
            {
                Audio::Play(ClipRef.Clip, volume, loop);
            }
        }
    };
}
#include "AudioSource.generated.h"
