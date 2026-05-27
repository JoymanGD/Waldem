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
    }
}
