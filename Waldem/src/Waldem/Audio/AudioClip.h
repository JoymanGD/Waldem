#pragma once

namespace Waldem
{
    class AudioClip
    {
    public:
        uint8_t* Data;
        Uint32 Length;
        uint8_t ChannelsNum; //1-mono, 2-stereo
        uint16_t Stride;
        AudioChannel* CurrentChannel;
    };
}
