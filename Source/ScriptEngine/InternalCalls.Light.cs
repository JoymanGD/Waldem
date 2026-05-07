using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Light_GetColor(ulong entityId, out Vector3 color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Light_SetColor(ulong entityId, ref Vector3 color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Light_GetIntensity(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Light_SetIntensity(ulong entityId, float intensity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Light_GetRadius(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Light_SetRadius(ulong entityId, float radius);
    }
}
