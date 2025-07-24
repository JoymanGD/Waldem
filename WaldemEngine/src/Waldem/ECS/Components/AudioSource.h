#pragma once
#include "Waldem/Audio/AudioClip.h"
#include "Waldem/Audio/Audio.h"

namespace Waldem
{
    struct WALDEM_API AudioSource
    {
        AudioClip* Clip = nullptr;
        float Range = 5.0f;
        float Volume = 1.0f;
        bool Loop = false;
        bool Spatial = true;
        bool PlayOnStart = false;

        AudioSource() = default;
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
