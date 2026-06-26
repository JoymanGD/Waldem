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
        internal static extern void RigidBody_ApplyImpulse(ulong entityId, ref Vector3 impulse);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_LookAt(ulong entityId, ref Vector3 target);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_RotateTowards(ulong entityId, ref Vector3 target, float maxDegreesDelta);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float RigidBody_GetMass(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetMass(ulong entityId, float mass);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetIsKinematic(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetIsKinematic(ulong entityId, bool isKinematic);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetFreezePositionX(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetFreezePositionX(ulong entityId, bool freeze);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetFreezePositionY(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetFreezePositionY(ulong entityId, bool freeze);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetFreezePositionZ(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetFreezePositionZ(ulong entityId, bool freeze);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetFreezeRotationX(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetFreezeRotationX(ulong entityId, bool freeze);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetFreezeRotationY(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetFreezeRotationY(ulong entityId, bool freeze);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool RigidBody_GetFreezeRotationZ(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void RigidBody_SetFreezeRotationZ(ulong entityId, bool freeze);
    }
}
