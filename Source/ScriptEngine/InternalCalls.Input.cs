using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsKeyPressed(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsMouseButtonPressed(int mouseButton);
    }
}
