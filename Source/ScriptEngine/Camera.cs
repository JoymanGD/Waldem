namespace Waldem
{
    public sealed class Camera : Component
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
        
        public static Camera Main
        {
            get
            {
                ulong entityId = InternalCalls.Camera_GetMainEntity();
                if (entityId == 0)
                    return null;

                if (main == null || main.EntityId != entityId)
                {
                    var camera = new Camera();
                    camera.Attach(entityId);
                    main = camera;
                }

                return main;
            }
        }

        private static Camera main;
    }
}
