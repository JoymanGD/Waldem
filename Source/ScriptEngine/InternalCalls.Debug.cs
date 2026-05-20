using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void LogInfo(string message);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void LogWarning(string message);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void LogError(string message);
    }
}