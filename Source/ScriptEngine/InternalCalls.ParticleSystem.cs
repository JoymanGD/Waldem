using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static partial class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern uint ParticleSystem_GetParticlesAmount(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetParticlesAmount(ulong entityId, uint particlesAmount);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_GetShape(ulong entityId, out ParticleSystemShape shape);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetShape(ulong entityId, ref ParticleSystemShape shape);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float ParticleSystem_GetRadius(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetRadius(ulong entityId, float radius);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool ParticleSystem_GetWorldSpace(ulong entityId);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetWorldSpace(ulong entityId, bool worldSpace);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_GetColor(ulong entityId, out Color color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetColor(ulong entityId, ref Color color);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_GetColorOverLifetime(ulong entityId, out Gradient colorOverLifetime);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetColorOverLifetime(ulong entityId, ref Gradient colorOverLifetime);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_GetSizeMinMax(ulong entityId, out Vector2 sizeMinMax);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetSizeMinMax(ulong entityId, ref Vector2 sizeMinMax);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_GetSpeedMinMax(ulong entityId, out Vector2 speedMinMax);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetSpeedMinMax(ulong entityId, ref Vector2 speedMinMax);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_GetLifetimeMinMax(ulong entityId, out Vector2 lifetimeMinMax);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetLifetimeMinMax(ulong entityId, ref Vector2 lifetimeMinMax);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_GetSizeOverLifeTime(ulong entityId, out Curve sizeOverLifetime);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ParticleSystem_SetSizeOverLifeTime(ulong entityId, ref Curve sizeOverLifetime);
    }
}
