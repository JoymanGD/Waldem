#pragma once
#include "Waldem/Audio/AudioClip.h"

namespace Waldem
{
    struct WALDEM_API AudioSource
    {
        AudioClip* Clip = nullptr;
        float Range = 5.0f;
        float Volume = 1.0f;
        bool Loop = false;
        bool Spatial = true;

        AudioSource(AudioClip* clip, float range, bool playOnStart, float volume = 1.0f, bool loop = false, bool spatial = true) : Clip(clip), Range(range), Volume(volume), Loop(loop), Spatial(spatial)
        {
            if(playOnStart)
            {
                Audio::Play(Clip, volume, loop);
            }
        }
    };
}
