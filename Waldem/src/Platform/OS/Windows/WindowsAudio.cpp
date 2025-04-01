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
        Uint8* wavData;
        Uint32 wavLength;
        
        if(p.extension() == ".wav")
        {
            if (SDL_LoadWAV(p.string().c_str(), &clipSpec, &wavData, &wavLength) == nullptr)
            {
                WD_CORE_ERROR("Failed to load WAV: {0}", SDL_GetError());
                return nullptr;
            }
        }
        else
        {
            WD_CORE_ERROR("Unsupported audio format: {0}", p.extension().string());
            return nullptr;
        }
            
        SDL_AudioCVT cvt;
        SDL_BuildAudioCVT(&cvt, clipSpec.format, clipSpec.channels, clipSpec.freq, WindowsAudio::Spec.format, WindowsAudio::Spec.channels, WindowsAudio::Spec.freq);

        SDL_AudioFormat format;
        
        if (cvt.needed)
        {
            cvt.len = wavLength;
            cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);
            SDL_memcpy(cvt.buf, wavData, wavLength);
            SDL_ConvertAudio(&cvt);

            clip->Data   = cvt.buf;
            clip->Length = cvt.len_cvt;
            SDL_FreeWAV(wavData);
            format = cvt.dst_format;
        }
        else
        {
            clip->Data   = wavData;
            clip->Length = wavLength;
            format = clipSpec.format;
        }
        
        clip->Stride = SDL_AUDIO_BITSIZE(format) / 8;
        clip->Channels = clipSpec.channels;
        
        return clip;
    }

    void WindowsAudio::AudioCallback(void* userdata, uint8_t* stream, int len)
    {
        float* out = reinterpret_cast<float*>(stream);
        int floatCount = len / sizeof(float);
        int framesToMix = floatCount / Spec.channels;  // since it's stereo (2 channels)

        SDL_memset(stream, 0, len);

        for (int ch = 0; ch < MAX_AUDIO_CHANNELS; ++ch)
        {
            AudioChannel& channel = Channels[ch];
            if (!channel.playing) continue;

            int framesLeft = channel.lengthInFrames - channel.position;
            int framesToCopy = (framesLeft < framesToMix) ? framesLeft : framesToMix;

            float* channelData = channel.samples + (channel.position * Spec.channels);

            for (int i = 0; i < framesToCopy * Spec.channels; ++i)
            {
                out[i] += channelData[i] * channel.volume;
            }

            channel.position += framesToCopy;

            if (channel.position >= channel.lengthInFrames)
            {
                if (channel.loop)
                {
                    channel.position = 0;
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
        desiredSpec.format = AUDIO_F32;
        desiredSpec.channels = 2;
        desiredSpec.samples = 1024;
        desiredSpec.callback = AudioCallback;
        desiredSpec.userdata = nullptr;

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
