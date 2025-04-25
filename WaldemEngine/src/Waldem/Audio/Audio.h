#pragma once

#include "AudioChannel.h"
#include "AudioClip.h"
#include "Waldem/Types/String.h"

static const int MAX_AUDIO_CHANNELS = 32;

namespace Waldem
{
	class WALDEM_API Audio
	{
	public:
		Audio() {}
		virtual ~Audio() {}

		static void Play(AudioClip* clip, float volume = 1.0f, bool loop = false);
		static void Pause(AudioClip* clip);
		static void Stop(AudioClip* clip);
		static void LockAudioThread();
		static void UnlockAudioThread();
		
		static AudioClip* Load(Path path, bool relative = true);
		static AudioChannel& GetChannel(int channel) { return Instance->Channels[channel]; }
		static Audio* Create();
        inline static Audio* Instance;
	protected:
		inline static AudioChannel Channels[MAX_AUDIO_CHANNELS];
	};
}
