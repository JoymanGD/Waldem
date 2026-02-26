#pragma once
#include <filesystem>

namespace Waldem
{
    inline uint64_t HashFromData(const void* data, size_t size)
    {
        std::string_view view(reinterpret_cast<const char*>(data), size);
        return std::hash<std::string_view>{}(view);
    }
}
