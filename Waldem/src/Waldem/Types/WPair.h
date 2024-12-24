#pragma once

namespace Waldem
{
    template<typename T1, typename T2>
    struct WALDEM_API WPair
    {
        T1 key;
        T2 value;

        WPair() = default;
        WPair(const T1& first, const T2& second) : key(first), value(second) {}
    };
}
