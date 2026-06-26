namespace Waldem
{
    public sealed class RigidBody : Component
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

        public bool FreezePositionX
        {
            get { return InternalCalls.RigidBody_GetFreezePositionX(EntityId); }
            set { InternalCalls.RigidBody_SetFreezePositionX(EntityId, value); }
        }

        public bool FreezePositionY
        {
            get { return InternalCalls.RigidBody_GetFreezePositionY(EntityId); }
            set { InternalCalls.RigidBody_SetFreezePositionY(EntityId, value); }
        }

        public bool FreezePositionZ
        {
            get { return InternalCalls.RigidBody_GetFreezePositionZ(EntityId); }
            set { InternalCalls.RigidBody_SetFreezePositionZ(EntityId, value); }
        }

        public bool FreezeRotationX
        {
            get { return InternalCalls.RigidBody_GetFreezeRotationX(EntityId); }
            set { InternalCalls.RigidBody_SetFreezeRotationX(EntityId, value); }
        }

        public bool FreezeRotationY
        {
            get { return InternalCalls.RigidBody_GetFreezeRotationY(EntityId); }
            set { InternalCalls.RigidBody_SetFreezeRotationY(EntityId, value); }
        }

        public bool FreezeRotationZ
        {
            get { return InternalCalls.RigidBody_GetFreezeRotationZ(EntityId); }
            set { InternalCalls.RigidBody_SetFreezeRotationZ(EntityId, value); }
        }

        public void AddForce(Vector3 force) { InternalCalls.RigidBody_AddForce(EntityId, ref force); }
        public void ApplyImpulse(Vector3 impulse) { InternalCalls.RigidBody_ApplyImpulse(EntityId, ref impulse); }
        public void LookAt(Vector3 target) { InternalCalls.RigidBody_LookAt(EntityId, ref target); }
        public void RotateTowards(Vector3 target, float maxDegreesDelta) { InternalCalls.RigidBody_RotateTowards(EntityId, ref target, maxDegreesDelta); }
    }
}
