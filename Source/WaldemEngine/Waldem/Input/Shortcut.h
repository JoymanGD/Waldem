#pragma once

#include <cstddef>
#include <functional>
#include <unordered_set>

#include "KeyCodes.h"

namespace Waldem
{
    struct Shortcut
    {
        std::unordered_set<int> Keys;

        bool Matches(const std::unordered_set<int>& pressed) const
        {
            const auto isCtrl = [](int key) { return key == LCTRL || key == RCTRL; };
            const auto isShift = [](int key) { return key == LSHIFT || key == RSHIFT; };
            const auto isAlt = [](int key) { return key == LALT || key == RALT; };
            const auto isSupportedModifier = [&](int key) { return isCtrl(key) || isShift(key) || isAlt(key); };

            const bool requiresCtrl = Keys.find(LCTRL) != Keys.end() || Keys.find(RCTRL) != Keys.end();
            const bool requiresShift = Keys.find(LSHIFT) != Keys.end() || Keys.find(RSHIFT) != Keys.end();
            const bool requiresAlt = Keys.find(LALT) != Keys.end() || Keys.find(RALT) != Keys.end();

            const bool hasCtrl = pressed.find(LCTRL) != pressed.end() || pressed.find(RCTRL) != pressed.end();
            const bool hasShift = pressed.find(LSHIFT) != pressed.end() || pressed.find(RSHIFT) != pressed.end();
            const bool hasAlt = pressed.find(LALT) != pressed.end() || pressed.find(RALT) != pressed.end();

            if(requiresCtrl != hasCtrl || requiresShift != hasShift || requiresAlt != hasAlt)
            {
                return false;
            }

            for(int key : Keys)
            {
                if(!isSupportedModifier(key) && pressed.find(key) == pressed.end())
                {
                    return false;
                }
            }

            for(int key : pressed)
            {
                if(!isSupportedModifier(key) && Keys.find(key) == Keys.end())
                {
                    return false;
                }
            }

            return true;
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
