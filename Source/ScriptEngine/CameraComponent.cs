namespace Waldem
{
    public sealed class CameraComponent : Component
    {
        public float FieldOfView
        {
            get { return InternalCalls.Camera_GetFieldOfView(EntityId); }
            set { InternalCalls.Camera_SetFieldOfView(EntityId, value); }
        }

        public float NearPlane
        {
            get { return InternalCalls.Camera_GetNearPlane(EntityId); }
            set { InternalCalls.Camera_SetNearPlane(EntityId, value); }
        }

        public float FarPlane
        {
            get { return InternalCalls.Camera_GetFarPlane(EntityId); }
            set { InternalCalls.Camera_SetFarPlane(EntityId, value); }
        }
    }
}
