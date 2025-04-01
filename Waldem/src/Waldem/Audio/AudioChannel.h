#pragma once

namespace Waldem
{
    struct AudioChannel
    {
        float* samples = nullptr; // pointer to decoded float samples
        int lengthInFrames = 0; // total frames (1 frame = 1 sample * channels?)
        int position = 0; // current frame playback position
        float volume = 0;
        bool playing = false;
        bool loop = false;
        bool paused = false;
    };
}