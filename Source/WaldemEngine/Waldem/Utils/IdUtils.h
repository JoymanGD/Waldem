#pragma once
#include <filesystem>
#include <random>

namespace Waldem
{
    inline uint64 GenerateUUID()
    {
        static std::random_device device;
        static std::mt19937_64 generator(device());

        uint64 uuid = 0;

        do
        {
            uuid = generator() & 0x7FFFFFFFFFFFFFFFULL;
        }
        while(uuid == 0 || uuid == 0x7FFFFFFFFFFFFFFFULL);

        return uuid;
    }
}
