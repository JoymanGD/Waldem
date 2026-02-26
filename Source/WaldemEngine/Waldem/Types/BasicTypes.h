#pragma once
#include <filesystem>

namespace Waldem
{
    using String = std::string;
    using Path = std::filesystem::path;
    using Action = std::function<void()>;
    using uint64 = uint64_t;
    using uint32 = uint32_t;
    using uint16 = uint16_t;
    using uint8 = uint8_t;
    using uint = uint32;
    using byte = unsigned char;
    
    #define UNLIKELY [[unlikely]]
    #define LIKELY [[likely]]
    #define likely_if(x) if (x) LIKELY
    #define unlikely_if(x) if (x) UNLIKELY
    #define likely_else else LIKELY
    #define unlikely_else else UNLIKELY
	#define BIND_ACTION(fn, ...) [this](auto&&... args) { fn(std::forward<decltype(args)>(args)..., ##__VA_ARGS__); }
	#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)
}
