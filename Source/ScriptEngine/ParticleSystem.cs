namespace Waldem
{
    public enum ParticleSystemShape
    {
        Circle = 0,
        Sphere = 1
    }

    public sealed class ParticleSystem : Component
    {
        public uint ParticlesAmount
        {
            get { return InternalCalls.ParticleSystem_GetParticlesAmount(EntityId); }
            set { InternalCalls.ParticleSystem_SetParticlesAmount(EntityId, value); }
        }

        public ParticleSystemShape Shape
        {
            get { InternalCalls.ParticleSystem_GetShape(EntityId, out ParticleSystemShape shape); return shape; }
            set { InternalCalls.ParticleSystem_SetShape(EntityId, ref value); }
        }

        public float Radius
        {
            get { return InternalCalls.ParticleSystem_GetRadius(EntityId); }
            set { InternalCalls.ParticleSystem_SetRadius(EntityId, value); }
        }

        public bool WorldSpace
        {
            get { return InternalCalls.ParticleSystem_GetWorldSpace(EntityId); }
            set { InternalCalls.ParticleSystem_SetWorldSpace(EntityId, value); }
        }

        public Color Color
        {
            get { InternalCalls.ParticleSystem_GetColor(EntityId, out Color color); return color; }
            set { InternalCalls.ParticleSystem_SetColor(EntityId, ref value); }
        }

        public Gradient ColorOverLifetime
        {
            get { InternalCalls.ParticleSystem_GetColorOverLifetime(EntityId, out Gradient colorOverLifetime); return colorOverLifetime; }
            set { InternalCalls.ParticleSystem_SetColorOverLifetime(EntityId, ref value); }
        }

        public Vector2 SizeMinMax
        {
            get { InternalCalls.ParticleSystem_GetSizeMinMax(EntityId, out Vector2 sizeMinMax); return sizeMinMax; }
            set { InternalCalls.ParticleSystem_SetSizeMinMax(EntityId, ref value); }
        }

        public Vector2 SpeedMinMax
        {
            get { InternalCalls.ParticleSystem_GetSpeedMinMax(EntityId, out Vector2 speedMinMax); return speedMinMax; }
            set { InternalCalls.ParticleSystem_SetSpeedMinMax(EntityId, ref value); }
        }

        public Vector2 LifetimeMinMax
        {
            get { InternalCalls.ParticleSystem_GetLifetimeMinMax(EntityId, out Vector2 lifetimeMinMax); return lifetimeMinMax; }
            set { InternalCalls.ParticleSystem_SetLifetimeMinMax(EntityId, ref value); }
        }

        public Curve SizeOverLifeTime
        {
            get { InternalCalls.ParticleSystem_GetSizeOverLifeTime(EntityId, out Curve sizeOverLifetime); return sizeOverLifetime; }
            set { InternalCalls.ParticleSystem_SetSizeOverLifeTime(EntityId, ref value); }
        }
    }
}
