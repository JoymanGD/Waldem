#include <wdpch.h>
#include "WindowsAudio.h"
#include <filesystem>

namespace Waldem
{
    Audio* Audio::Create()
    {
        Instance = new WindowsAudio();
        
        return Instance;
    }

    std::filesystem::path GetPathToSound(std::filesystem::path path)
    {
        WArray<String> exts = { ".wav", ".ogg", ".mp3" };

        for (auto& ext : exts)
        {
            auto candidate = path;
            candidate.replace_extension(ext);
            if (exists(candidate))
            {
                return candidate;
            }
        }

        WD_CORE_ERROR("No file found for base path:  {0}", path);

        return path;
    }

    AudioClip* Audio::Load(String path, bool relative)
    {
        std::filesystem::path p = path;

        if(relative)
        {
            p = std::filesystem::current_path() / p;
        }

        p = GetPathToSound(p);
        
        AudioClip* clip = new AudioClip();
        
        SDL_AudioSpec clipSpec;
        
        if(p.extension() == ".wav")
        {
            if (SDL_LoadWAV(p.string().c_str(), &clipSpec, &clip->Data, &clip->Length) == nullptr)
            {
                WD_CORE_ERROR("Failed to load WAV: {0}", SDL_GetError());
                return nullptr;
            }
        }
        else if(p.extension() == ".mp3")
        {
            WD_CORE_ERROR("mp3 format is not yet supported");
            return nullptr;
        }
        else
        {
            WD_CORE_ERROR("Unsupported audio format: {0}", p.extension().string());
            return nullptr;
        }

        clip->Stride = SDL_AUDIO_BITSIZE(clipSpec.format) / 8;
        clip->Channels = clipSpec.channels;
        
        return clip;
    }

    void WindowsAudio::AudioCallback(void* userdata, uint8_t* stream, int len)
    {
        float* out = reinterpret_cast<float*>(stream);
        int floatCount = len / sizeof(float);
        int framesToMix = floatCount / Spec.channels;  // since it's stereo (2 channels)

        // 1) Clear output
        SDL_memset(stream, 0, len);

        // 2) Mix each active channel
        for (int ch = 0; ch < MAX_AUDIO_CHANNELS; ++ch)
        {
            AudioChannel& channel = Channels[ch];
            if (!channel.playing) continue;

            // how many frames remain in this channel?
            int framesLeft = channel.lengthInFrames - channel.position;
            int framesToCopy = (framesLeft < framesToMix) ? framesLeft : framesToMix;

            // pointer to our channel's data at its current position
            float* channelData = channel.samples + (channel.position * Spec.channels);

            // accumulate into out
            for (int i = 0; i < framesToCopy * Spec.channels; ++i)
            {
                out[i] += channelData[i] * channel.volume;
            }

            // advance the channel position
            channel.position += framesToCopy;

            // if we've reached the end
            if (channel.position >= channel.lengthInFrames)
            {
                if (channel.loop)
                {
                    channel.position = 0;  // loop from start
                }
                else
                {
                    channel.playing = false;
                }
            }
        }
    }

    WindowsAudio::WindowsAudio()
    {
        SDL_AudioSpec desiredSpec;
        SDL_zero(desiredSpec);

        desiredSpec.freq = 44100;
        desiredSpec.format = AUDIO_F32;     // 32-bit float samples
        desiredSpec.channels = 2;          // stereo
        desiredSpec.samples = 1024;        // buffer size
        desiredSpec.callback = AudioCallback;
        desiredSpec.userdata = nullptr;    // we can pass a custom pointer if needed

        Device = SDL_OpenAudioDevice(
            nullptr,
            0,
            &desiredSpec,
            &Spec,
            0
        );

        if (Device == 0)
        {
            WD_CORE_ERROR("Failed to open audio device: {0}", SDL_GetError());
        }
        
        SDL_PauseAudioDevice(Device, 0);
    }

    void Audio::Play(AudioClip* clip, float volume, bool loop)
    {
        // find a free channel
        SDL_LockAudioDevice(WindowsAudio::Device);
        for (int i = 0; i < MAX_AUDIO_CHANNELS; ++i)
        {
            if (!Channels[i].playing)
            {
                Channels[i].samples = (float*)clip->Data;
                Channels[i].lengthInFrames = (int)clip->Length / (clip->Stride * clip->Channels);
                Channels[i].position = 0;
                Channels[i].volume = volume;
                Channels[i].loop = loop;
                Channels[i].playing = true;
                break;
            }
        }
        SDL_UnlockAudioDevice(WindowsAudio::Device);
    }

    void Audio::Pause()
    {
    }
}
