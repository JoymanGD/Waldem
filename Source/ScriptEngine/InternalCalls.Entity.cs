using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Entity_HasComponent(ulong entityId, int componentKind);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Entity_AddComponent(ulong entityId, int componentKind);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Entity_Destroy(ulong entityId);
    }
}
