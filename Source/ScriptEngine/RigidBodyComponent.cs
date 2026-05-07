namespace Waldem
{
    public sealed class RigidBodyComponent : Component
    {
        public Vector3 Velocity
        {
            get { InternalCalls.RigidBody_GetVelocity(EntityId, out Vector3 v); return v; }
            set { InternalCalls.RigidBody_SetVelocity(EntityId, ref value); }
        }

        public Vector3 AngularVelocity
        {
            get { InternalCalls.RigidBody_GetAngularVelocity(EntityId, out Vector3 v); return v; }
            set { InternalCalls.RigidBody_SetAngularVelocity(EntityId, ref value); }
        }

        public float Mass
        {
            get { return InternalCalls.RigidBody_GetMass(EntityId); }
            set { InternalCalls.RigidBody_SetMass(EntityId, value); }
        }

        public bool IsKinematic
        {
            get { return InternalCalls.RigidBody_GetIsKinematic(EntityId); }
            set { InternalCalls.RigidBody_SetIsKinematic(EntityId, value); }
        }

        public bool IsGrounded => InternalCalls.RigidBody_GetIsGrounded(EntityId);

        public void AddForce(Vector3 force) { InternalCalls.RigidBody_AddForce(EntityId, ref force); }
    }
}
