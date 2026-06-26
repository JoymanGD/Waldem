namespace Waldem
{
    public class CharacterController : Component
    {
        public Vector3 MoveVelocity
        {
            get { InternalCalls.CharacterController_GetMoveVelocity(EntityId, out Vector3 velocity); return velocity; }
            set { InternalCalls.CharacterController_SetMoveVelocity(EntityId, ref value); }
        }

        public bool IsGrounded => InternalCalls.CharacterController_GetIsGrounded(EntityId);

        public float JumpSpeed
        {
            get { return InternalCalls.CharacterController_GetJumpSpeed(EntityId); }
            set { InternalCalls.CharacterController_SetJumpSpeed(EntityId, value); }
        }

        public void Jump()
        {
            InternalCalls.CharacterController_Jump(EntityId);
        }
    }
}
