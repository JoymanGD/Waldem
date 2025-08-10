#pragma once
#include "Waldem/Audio/Audio.h"
#include "ComponentBase.h"
#include "Waldem/Editor/AssetReference/AudioClipReference.h"

namespace Waldem
{
    struct WALDEM_API AudioSource
    {
        COMPONENT(AudioSource)
            FIELD(float, Range)
            FIELD(float, Volume)
            FIELD(bool, Loop)
            FIELD(bool, Spatial)
            FIELD(bool, PlayOnStart)
            FIELD(AssetReference, Clip)
        END_COMPONENT()
        
        float Range = 5.0f;
        float Volume = 1.0f;
        bool Loop = false;
        bool Spatial = true;
        bool PlayOnStart = false;
        AudioClipReference ClipRef;

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
