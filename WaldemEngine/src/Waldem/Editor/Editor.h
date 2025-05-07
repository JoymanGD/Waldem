#pragma once
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    using ResizeCallback = std::function<void(Vector2 size)>;
    
    class WALDEM_API Editor
    {
    public:
        inline static int HoveredEntityID;
        inline static bool IsMouseOverEditorViewport = false;
        inline static Vector2 EditorViewportSize;
        inline static Vector2 EditorViewportPos;
        inline static Vector2 EditorViewportMousePos;
        inline static bool GizmoIsUsing = false;

        static void Resize(Vector2 size)
        {
            for (auto& callback : ResizeCallbacks)
            {
                callback(size);
            }
        }

        static void SubscribeOnResize(const ResizeCallback& callback)
        {
            ResizeCallbacks.Add(callback);
        }

    private:
        inline static WArray<ResizeCallback> ResizeCallbacks;
    };
}
