using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_GetVelocity(ulong entityId, out Vector3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetVelocity(ulong entityId, ref Vector3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_GetAngularVelocity(ulong entityId, out Vector3 angularVelocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetAngularVelocity(ulong entityId, ref Vector3 angularVelocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_AddForce(ulong entityId, ref Vector3 force);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetIsGrounded(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float RigidBody_GetMass(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetMass(ulong entityId, float mass);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetIsKinematic(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetIsKinematic(ulong entityId, bool isKinematic);
    }
}
