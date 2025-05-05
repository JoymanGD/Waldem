#pragma once

namespace Waldem
{
    class WALDEM_API Editor
    {
    public:
        inline static int HoveredEntityID;
        inline static bool IsMouseOverEditorViewport = false;
        inline static Vector2 EditorViewportSize;
        inline static Vector2 EditorViewportPos;
    };
}
