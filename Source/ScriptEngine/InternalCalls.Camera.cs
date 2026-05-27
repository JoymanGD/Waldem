using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Camera_GetFieldOfView(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Camera_SetFieldOfView(ulong entityId, float fov);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Camera_GetNearPlane(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Camera_SetNearPlane(ulong entityId, float nearPlane);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Camera_GetFarPlane(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Camera_SetFarPlane(ulong entityId, float farPlane);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong Camera_GetMainEntity();
    }
}
