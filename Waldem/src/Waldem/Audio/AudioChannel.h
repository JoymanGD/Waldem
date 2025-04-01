#pragma once

namespace Waldem
{
    struct AudioChannel
    {
        float* samples = nullptr; // pointer to decoded float samples
        int lengthInFrames = 0; // total frames (1 frame = 1 sample * channels?)
        int position = 0; // current frame playback position
        float volume = 1;
        float distanceVolume = 1;
        float pan = 0; //[-1; 1]
        bool playing = false;
        bool loop = false;
        bool paused = false;
    };
}