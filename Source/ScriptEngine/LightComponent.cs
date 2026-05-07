namespace Waldem
{
    public sealed class LightComponent : Component
    {
        public Vector3 Color
        {
            get { InternalCalls.Light_GetColor(EntityId, out Vector3 c); return c; }
            set { InternalCalls.Light_SetColor(EntityId, ref value); }
        }

        public float Intensity
        {
            get { return InternalCalls.Light_GetIntensity(EntityId); }
            set { InternalCalls.Light_SetIntensity(EntityId, value); }
        }

        public float Radius
        {
            get { return InternalCalls.Light_GetRadius(EntityId); }
            set { InternalCalls.Light_SetRadius(EntityId, value); }
        }
    }
}
