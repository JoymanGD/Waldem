#pragma once
#include <SDL_audio.h>
#include "Waldem/Audio/Audio.h"

namespace Waldem
{
    class WALDEM_API WindowsAudio : public Audio
    {
    public:
        WindowsAudio();
        inline static SDL_AudioDeviceID Device;
        inline static SDL_AudioSpec Spec;
        static void AudioCallback(void* userdata, uint8_t* stream, int len);
    };
}