using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Quaternion_Normalize(ref Quaternion value, out Quaternion result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Quaternion_Multiply(ref Quaternion left, ref Quaternion right, out Quaternion result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Quaternion_RotateVector(ref Quaternion rotation, ref Vector3 point, out Vector3 result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Quaternion_Euler(ref Vector3 eulerDegrees, out Quaternion result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Quaternion_AngleAxis(float angleDegrees, ref Vector3 axis, out Quaternion result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Quaternion_LookRotation(ref Vector3 forward, ref Vector3 up, out Quaternion result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Quaternion_ToEuler(ref Quaternion value, out Vector3 result);
    }
}
