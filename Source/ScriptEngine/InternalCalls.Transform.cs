using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetPosition(ulong entityId, out Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetPosition(ulong entityId, ref Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetForward(ulong entityId, out Vector3 forward);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetForward(ulong entityId, Vector3 forward);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetUp(ulong entityId, out Vector3 up);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetRight(ulong entityId, out Vector3 right);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_Translate(ulong entityId, ref Vector3 translation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetRotation(ulong entityId, out Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetRotation(ulong entityId, ref Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_Rotate(ulong entityId, ref Vector3 rotationDelta);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetRotationQuaternion(ulong entityId, out Quaternion rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetRotationQuaternion(ulong entityId, ref Quaternion rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_RotateQuaternion(ulong entityId, ref Quaternion rotationDelta);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetScale(ulong entityId, out Vector3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetScale(ulong entityId, ref Vector3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_LookAt(ulong entityId, ref Vector3 target);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_LookAtWithUp(ulong entityId, ref Vector3 target, ref Vector3 up);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_RotateAround(ulong entityId, ref Vector3 point, ref Vector3 axis, float angleDegrees);
    }
}
