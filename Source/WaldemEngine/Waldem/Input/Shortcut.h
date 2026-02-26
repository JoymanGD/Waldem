#pragma once

namespace Waldem
{
    struct Shortcut
    {
        std::unordered_set<int> Keys;

        bool Matches(const std::unordered_set<int>& pressed) const
        {
            return Keys == pressed;
        }

        bool operator==(const Shortcut& other) const noexcept
        {
            return Keys == other.Keys;
        }
    };
    
    struct ShortcutHash
    {
        std::size_t operator()(const Shortcut& s) const noexcept
        {
            std::size_t hash = 0;
            for (int key : s.Keys)
            {
                hash ^= std::hash<int>()(key) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
}
