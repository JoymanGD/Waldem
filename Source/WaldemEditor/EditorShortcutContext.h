#pragma once

#include <cstdint>

namespace Waldem
{
    enum class EditorShortcutContext : uint32_t
    {
        None = 0,
        EditorViewport = 1 << 0,
        GameViewport = 1 << 1,
        Hierarchy = 1 << 2,
        ContentBrowser = 1 << 3
    };

    class EditorShortcutContexts
    {
    public:
        static void BeginFrame()
        {
            ActiveContexts = 0;
        }

        static void SetActive(EditorShortcutContext context, bool active)
        {
            const uint32_t mask = static_cast<uint32_t>(context);
            if(active)
            {
                ActiveContexts |= mask;
            }
            else
            {
                ActiveContexts &= ~mask;
            }
        }

        static bool Has(EditorShortcutContext context)
        {
            const uint32_t mask = static_cast<uint32_t>(context);
            return (ActiveContexts & mask) == mask;
        }

    private:
        inline static uint32_t ActiveContexts = 0;
    };
}
