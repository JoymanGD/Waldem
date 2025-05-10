#pragma once

#include <SDL_audio.h>
#include "AudioChannel.h"
#include "AudioClip.h"
#include "Waldem/Types/String.h"

static const int MAX_AUDIO_CHANNELS = 32;

namespace Waldem
{
	class WALDEM_API Audio
	{
	public:
		inline static Audio* Instance;
		inline static SDL_AudioSpec Spec;
		
		Audio()
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

			Instance = this;
		}
		
		virtual ~Audio() {}

		static void Play(AudioClip* clip, float volume = 1.0f, bool loop = false)
		{
			if(!clip)
			{
				WD_CORE_ERROR("Audio clip is null");
				return;
			}
        
			SDL_LockAudioDevice(Device);

			if(clip->CurrentChannel != nullptr && clip->CurrentChannel->playing && clip->CurrentChannel->paused)
			{
				clip->CurrentChannel->paused = false;
			}
			else
			{
				for (int i = 0; i < MAX_AUDIO_CHANNELS; ++i)
				{
					if (!Channels[i].playing)
					{
						Channels[i].samples = (float*)clip->Data;
						Channels[i].lengthInFrames = (int)clip->Length / (clip->Stride * clip->ChannelsNum);
						Channels[i].position = 0;
						Channels[i].volume = volume;
						Channels[i].loop = loop;
						Channels[i].playing = true;
						Channels[i].paused = false;

						clip->CurrentChannel = &Channels[i];
                    
						break;
					}
				}
			}
        
			SDL_UnlockAudioDevice(Device);
		}
		
		static void Pause(AudioClip* clip)
		{
			if(clip->CurrentChannel && clip->CurrentChannel->playing)
			{
				SDL_LockAudioDevice(Device);
				clip->CurrentChannel->paused = true;
				SDL_UnlockAudioDevice(Device);
			}
		}
		
		static void Stop(AudioClip* clip)
		{
			SDL_LockAudioDevice(Device);
			clip->CurrentChannel->playing = false;
			clip->CurrentChannel->position = 0;
			clip->CurrentChannel = nullptr;
			SDL_UnlockAudioDevice(Device);
		}
		
		static void LockAudioThread()
		{
			SDL_LockAudioDevice(Device);
		}
		
		static void UnlockAudioThread()
		{
	        SDL_UnlockAudioDevice(Device);
		}
		
		static AudioChannel& GetChannel(int channel) { return Instance->Channels[channel]; }
		
	protected:
		inline static AudioChannel Channels[MAX_AUDIO_CHANNELS];
		inline static SDL_AudioDeviceID Device;
		
		static void AudioCallback(void* userdata, uint8_t* stream, int len)
		{
			float* out = reinterpret_cast<float*>(stream);
			int floatCount = len / sizeof(float);
			int framesToMix = floatCount / Spec.channels;  // since it's stereo (2 channels)

			SDL_memset(stream, 0, len);

			for (int ch = 0; ch < MAX_AUDIO_CHANNELS; ++ch)
			{
				AudioChannel& channel = Channels[ch];
				if (!channel.playing) continue;
				if (channel.paused) continue;

				int framesLeft = channel.lengthInFrames - channel.position;
				int framesToCopy = (framesLeft < framesToMix) ? framesLeft : framesToMix;

				float* channelData = channel.samples + (channel.position * Spec.channels);

				float leftVolume = channel.distanceVolume * channel.volume * (1.0f - channel.pan) / 2.0f;
				float rightVolume = channel.distanceVolume * channel.volume * (1.0f + channel.pan) / 2.0f;

				for (int frame = 0; frame < framesToCopy; ++frame)
				{
					// The source's left and right samples for this frame
					float sampleL = channelData[frame * 2 + 0];
					float sampleR = channelData[frame * 2 + 1];
                
					// Multiply by the channel's panning volumes
					out[frame * 2 + 0] += sampleL * leftVolume;
					out[frame * 2 + 1] += sampleR * rightVolume;
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
	};
}
