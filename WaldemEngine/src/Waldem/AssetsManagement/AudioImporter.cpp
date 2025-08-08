#include "wdpch.h"
#include "AudioImporter.h"

#include <SDL_audio.h>
#include "Waldem/Audio/Audio.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/WArray.h"
#include "Waldem/Utils/AssetUtils.h"

namespace Waldem
{
    CAudioImporter::CAudioImporter()
    {
    }

    Path GetPathToSound(Path path)
    {
        WArray<Path> exts = { ".wav", ".ogg", ".mp3" };

        for (auto& ext : exts)
        {
            auto candidate = path;
            candidate.replace_extension(ext);
            if (exists(candidate))
            {
                return candidate;
            }
        }

        WD_CORE_ERROR("No sound file found for base path:  {0}", path);

        return path;
    }

    AudioClip* CAudioImporter::Import(const Path& path, bool relative)
    {
        Path p = GetPathForAsset(AssetType::Audio);
        p /= path;
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
        SDL_BuildAudioCVT(&cvt, clipSpec.format, clipSpec.channels, clipSpec.freq, Audio::Spec.format, Audio::Spec.channels, Audio::Spec.freq);

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
        clip->ChannelsNum = clipSpec.channels;
        clip->Name = path.filename().string();
        
        return clip;
    }
}
