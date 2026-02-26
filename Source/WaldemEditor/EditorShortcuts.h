#pragma once

#include <array>
#include <fstream>
#include <sstream>

#include "Waldem/Input/KeyCodes.h"
#include "Waldem/Input/Shortcut.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    enum class EditorShortcutAction : int
    {
        Undo = 0,
        Redo,
        DuplicateEntity,
        RenameEntity,
        DeleteEntity,
        GizmoTranslate,
        GizmoRotate,
        GizmoScale,
        GizmoToggleMode,
        Count
    };

    struct ShortcutBinding
    {
        bool Ctrl = false;
        bool Shift = false;
        bool Alt = false;
        int Key = UNKNOWN;

        Shortcut ToShortcut() const
        {
            Shortcut shortcut{};

            if(Ctrl) shortcut.Keys.insert(LCTRL);
            if(Shift) shortcut.Keys.insert(LSHIFT);
            if(Alt) shortcut.Keys.insert(LALT);
            if(Key != UNKNOWN) shortcut.Keys.insert(Key);

            return shortcut;
        }

        static ShortcutBinding FromShortcut(const Shortcut& shortcut)
        {
            ShortcutBinding binding{};

            binding.Ctrl = shortcut.Keys.find(LCTRL) != shortcut.Keys.end();
            binding.Shift = shortcut.Keys.find(LSHIFT) != shortcut.Keys.end();
            binding.Alt = shortcut.Keys.find(LALT) != shortcut.Keys.end();

            for (int key : shortcut.Keys)
            {
                if(key != LCTRL && key != LSHIFT && key != LALT)
                {
                    binding.Key = key;
                    break;
                }
            }

            return binding;
        }
    };

    class EditorShortcuts
    {
    public:
        static Shortcut GetShortcut(EditorShortcutAction action)
        {
            return Bindings[(int)action].ToShortcut();
        }

        static ShortcutBinding GetBinding(EditorShortcutAction action)
        {
            return Bindings[(int)action];
        }

        static void SetBinding(EditorShortcutAction action, const ShortcutBinding& binding)
        {
            Bindings[(int)action] = binding;
        }

        static const char* GetActionName(EditorShortcutAction action)
        {
            return ActionNames[(int)action];
        }

        static int ActionCount()
        {
            return (int)EditorShortcutAction::Count;
        }

        static EditorShortcutAction ActionByIndex(int index)
        {
            return (EditorShortcutAction)index;
        }

        static void ResetDefaults()
        {
            Bindings = DefaultBindings;
        }

        static void Save(const Path& path = GetDefaultPath())
        {
            std::ofstream out(path);
            if(!out.is_open())
            {
                return;
            }

            for (int i = 0; i < ActionCount(); ++i)
            {
                const ShortcutBinding& b = Bindings[i];
                out << i << " " << (int)b.Ctrl << " " << (int)b.Shift << " " << (int)b.Alt << " " << b.Key << "\n";
            }
        }

        static void Load(const Path& path = GetDefaultPath())
        {
            std::ifstream in(path);
            if(!in.is_open())
            {
                return;
            }

            int index = 0, ctrl = 0, shift = 0, alt = 0, key = 0;
            while (in >> index >> ctrl >> shift >> alt >> key)
            {
                if(index >= 0 && index < ActionCount())
                {
                    Bindings[index] = { ctrl != 0, shift != 0, alt != 0, key };
                }
            }
        }

        static Path GetDefaultPath()
        {
            return "Content/EditorShortcuts.cfg";
        }

    private:
        inline static const std::array<const char*, (int)EditorShortcutAction::Count> ActionNames =
        {
            "Undo",
            "Redo",
            "Duplicate Entity",
            "Rename Entity",
            "Delete Entity",
            "Gizmo Translate",
            "Gizmo Rotate",
            "Gizmo Scale",
            "Gizmo Toggle Mode"
        };

        inline static const std::array<ShortcutBinding, (int)EditorShortcutAction::Count> DefaultBindings =
        {
            ShortcutBinding{ true, false, false, Z },
            ShortcutBinding{ true, true, false, Z },
            ShortcutBinding{ true, false, false, D },
            ShortcutBinding{ false, false, false, F2 },
            ShortcutBinding{ false, false, false, KEY_DELETE },
            ShortcutBinding{ false, false, false, W },
            ShortcutBinding{ false, false, false, E },
            ShortcutBinding{ false, false, false, R },
            ShortcutBinding{ false, false, false, Q }
        };

        inline static std::array<ShortcutBinding, (int)EditorShortcutAction::Count> Bindings = DefaultBindings;
    };
}
