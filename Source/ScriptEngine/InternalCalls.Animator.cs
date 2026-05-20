using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Animator_Play(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Animator_Stop(ulong entityId);
    }
}
