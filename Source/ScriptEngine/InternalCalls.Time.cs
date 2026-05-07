using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Time_GetDeltaTime();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Time_GetFixedDeltaTime();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Time_GetElapsedTime();
    }
}
