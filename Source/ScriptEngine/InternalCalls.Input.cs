using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Input_SetCursor(bool enable);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_GetKey(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_GetKeyDown(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_GetMouse(int mouseButton);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_GetMouseDown(int mouseButton);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Input_GetMouseDeltaX();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Input_GetMouseDeltaY();
    }
}
