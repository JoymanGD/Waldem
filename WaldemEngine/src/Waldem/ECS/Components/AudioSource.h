#pragma once
#include "Waldem/Audio/AudioClip.h"
#include "Waldem/Audio/Audio.h"
#include "ComponentBase.h"

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
        END_COMPONENT()
        
        float Range = 5.0f;
        float Volume = 1.0f;
        bool Loop = false;
        bool Spatial = true;
        bool PlayOnStart = false;
        AudioClip* Clip = nullptr;

        AudioSource(AudioClip* clip, float range, bool playOnStart, float volume = 1.0f, bool loop = false, bool spatial = true) : Clip(clip), Range(range), Volume(volume), Loop(loop), Spatial(spatial)
        {
            PlayOnStart = playOnStart;
            
            if(PlayOnStart)
            {
                Audio::Play(Clip, volume, loop);
            }
        }
    };
}
