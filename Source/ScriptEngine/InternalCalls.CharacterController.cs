using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void CharacterController_GetMoveVelocity(ulong entityId, out Vector3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void CharacterController_SetMoveVelocity(ulong entityId, ref Vector3 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool CharacterController_GetIsGrounded(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float CharacterController_GetJumpSpeed(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void CharacterController_SetJumpSpeed(ulong entityId, float jumpSpeed);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void CharacterController_Jump(ulong entityId);
    }
}
