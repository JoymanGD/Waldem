namespace Waldem
{
    public sealed class Transform : Component
    {
        public Vector3 Position
        {
            get
            {
                InternalCalls.Transform_GetPosition(EntityId, out Vector3 position);
                return position;
            }
            set
            {
                InternalCalls.Transform_SetPosition(EntityId, ref value);
            }
        }
        
        public Vector3 Forward
        {
            get
            {
                InternalCalls.Transform_GetForward(EntityId, out Vector3 forward);
                return forward;
            }
            set
            {
                InternalCalls.Transform_SetForward(EntityId, value);
            }
        }
        
        public Vector3 Up
        {
            get
            {
                InternalCalls.Transform_GetUp(EntityId, out Vector3 up);
                return up;
            }
        }
        
        public Vector3 Right
        {
            get
            {
                InternalCalls.Transform_GetRight(EntityId, out Vector3 right);
                return right;
            }
        }

        public void Translate(Vector3 translation)
        {
            InternalCalls.Transform_Translate(EntityId, ref translation);
        }

        public Vector3 Rotation
        {
            get
            {
                InternalCalls.Transform_GetRotation(EntityId, out Vector3 rotation);
                return rotation;
            }
            set
            {
                InternalCalls.Transform_SetRotation(EntityId, ref value);
            }
        }

        public Quaternion RotationQuaternion
        {
            get
            {
                InternalCalls.Transform_GetRotationQuaternion(EntityId, out Quaternion rotation);
                return rotation;
            }
            set
            {
                InternalCalls.Transform_SetRotationQuaternion(EntityId, ref value);
            }
        }

        public Vector3 Scale
        {
            get
            {
                InternalCalls.Transform_GetScale(EntityId, out Vector3 scale);
                return scale;
            }
            set
            {
                InternalCalls.Transform_SetScale(EntityId, ref value);
            }
        }

        public void Rotate(Vector3 rotationDelta)
        {
            InternalCalls.Transform_Rotate(EntityId, ref rotationDelta);
        }

        public void Rotate(Quaternion rotationDelta)
        {
            InternalCalls.Transform_RotateQuaternion(EntityId, ref rotationDelta);
        }

        public void LookAt(Vector3 target)
        {
            InternalCalls.Transform_LookAt(EntityId, ref target);
        }

        public void LookAt(Vector3 target, Vector3 up)
        {
            InternalCalls.Transform_LookAtWithUp(EntityId, ref target, ref up);
        }

        public void RotateAround(Vector3 point, Vector3 axis, float angleDegrees)
        {
            InternalCalls.Transform_RotateAround(EntityId, ref point, ref axis, angleDegrees);
        }
    }
}
