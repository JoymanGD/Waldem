#include "wdpch.h"
#include "ParticleSystemBindings.h"
#include "ScriptBindings.h"
#include "Waldem/ECS/Components/ParticleSystemComponent.h"
#include "Waldem/Scripting/Mono.h"

namespace Waldem::Bindings
{
    namespace
    {
        struct ScriptVector2 { float x = 0.0f, y = 0.0f; };
        struct ScriptVector4 { float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f; };
        struct ScriptColor { float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f; };

        struct ScriptGradient
        {
            ScriptVector4 Points[WGradient::MaxPoints];
            uint Count = 0;
            uint Padding[3] = {};
        };

        struct ScriptCurve
        {
            ScriptVector2 Points[WCurve::MaxPoints];
            ScriptVector2 Min;
            ScriptVector2 Max;
            uint Count = 0;
            uint Padding[3] = {};
        };

        static_assert(sizeof(ScriptGradient) == sizeof(WGradient));
        static_assert(sizeof(ScriptCurve) == sizeof(WCurve));

        uint ParticleSystem_GetParticlesAmount(uint64 entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return 0;
            return entity.get<ParticleSystemComponent>().ParticlesAmount;
        }

        void ParticleSystem_SetParticlesAmount(uint64 entityId, uint particlesAmount)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            entity.get_mut<ParticleSystemComponent>().ParticlesAmount = particlesAmount;
            entity.modified<ParticleSystemComponent>();
        }

        void ParticleSystem_GetShape(uint64 entityId, int32_t* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) { *out = 0; return; }
            *out = (int32_t)entity.get<ParticleSystemComponent>().Shape;
        }

        void ParticleSystem_SetShape(uint64 entityId, int32_t* shape)
        {
            if(shape == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            entity.get_mut<ParticleSystemComponent>().Shape = (EmitterShape)*shape;
            entity.modified<ParticleSystemComponent>();
        }

        float ParticleSystem_GetRadius(uint64 entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return 0.0f;
            return entity.get<ParticleSystemComponent>().Radius;
        }

        void ParticleSystem_SetRadius(uint64 entityId, float radius)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            entity.get_mut<ParticleSystemComponent>().Radius = radius;
            entity.modified<ParticleSystemComponent>();
        }

        bool ParticleSystem_GetWorldSpace(uint64 entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return false;
            return entity.get<ParticleSystemComponent>().WorldSpace;
        }

        void ParticleSystem_SetWorldSpace(uint64 entityId, bool worldSpace)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            entity.get_mut<ParticleSystemComponent>().WorldSpace = worldSpace;
            entity.modified<ParticleSystemComponent>();
        }

        void ParticleSystem_GetColor(uint64 entityId, ScriptColor* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) { *out = {}; return; }
            const auto& color = entity.get<ParticleSystemComponent>().Color;
            out->r = color.r; out->g = color.g; out->b = color.b; out->a = color.a;
        }

        void ParticleSystem_SetColor(uint64 entityId, ScriptColor* color)
        {
            if(color == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            auto& target = entity.get_mut<ParticleSystemComponent>().Color;
            target.r = color->r; target.g = color->g; target.b = color->b; target.a = color->a;
            entity.modified<ParticleSystemComponent>();
        }

        void ParticleSystem_GetColorOverLifetime(uint64 entityId, ScriptGradient* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) { *out = {}; return; }

            const auto& gradient = entity.get<ParticleSystemComponent>().ColorOverLifetime;
            out->Count = gradient.Count;

            for(uint i = 0; i < WGradient::MaxPoints; ++i)
            {
                out->Points[i].x = gradient.Points[i].x;
                out->Points[i].y = gradient.Points[i].y;
                out->Points[i].z = gradient.Points[i].z;
                out->Points[i].w = gradient.Points[i].w;
            }
        }

        void ParticleSystem_SetColorOverLifetime(uint64 entityId, ScriptGradient* gradient)
        {
            if(gradient == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;

            auto& target = entity.get_mut<ParticleSystemComponent>().ColorOverLifetime;
            target.Count = std::min(gradient->Count, WGradient::MaxPoints);

            for(uint i = 0; i < WGradient::MaxPoints; ++i)
            {
                target.Points[i] = Vector4(gradient->Points[i].x, gradient->Points[i].y, gradient->Points[i].z, gradient->Points[i].w);
            }

            entity.modified<ParticleSystemComponent>();
        }

        void ParticleSystem_GetSizeMinMax(uint64 entityId, ScriptVector2* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) { *out = {}; return; }
            const auto& value = entity.get<ParticleSystemComponent>().SizeMinMax;
            out->x = value.x; out->y = value.y;
        }

        void ParticleSystem_SetSizeMinMax(uint64 entityId, ScriptVector2* value)
        {
            if(value == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            entity.get_mut<ParticleSystemComponent>().SizeMinMax = Vector2(value->x, value->y);
            entity.modified<ParticleSystemComponent>();
        }

        void ParticleSystem_GetSpeedMinMax(uint64 entityId, ScriptVector2* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) { *out = {}; return; }
            const auto& value = entity.get<ParticleSystemComponent>().SpeedMinMax;
            out->x = value.x; out->y = value.y;
        }

        void ParticleSystem_SetSpeedMinMax(uint64 entityId, ScriptVector2* value)
        {
            if(value == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            entity.get_mut<ParticleSystemComponent>().SpeedMinMax = Vector2(value->x, value->y);
            entity.modified<ParticleSystemComponent>();
        }

        void ParticleSystem_GetLifetimeMinMax(uint64 entityId, ScriptVector2* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) { *out = {}; return; }
            const auto& value = entity.get<ParticleSystemComponent>().LifetimeMinMax;
            out->x = value.x; out->y = value.y;
        }

        void ParticleSystem_SetLifetimeMinMax(uint64 entityId, ScriptVector2* value)
        {
            if(value == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;
            entity.get_mut<ParticleSystemComponent>().LifetimeMinMax = Vector2(value->x, value->y);
            entity.modified<ParticleSystemComponent>();
        }

        void ParticleSystem_GetSizeOverLifeTime(uint64 entityId, ScriptCurve* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) { *out = {}; return; }

            const auto& curve = entity.get<ParticleSystemComponent>().SizeOverLifeTime;
            out->Min.x = curve.Min.x; out->Min.y = curve.Min.y;
            out->Max.x = curve.Max.x; out->Max.y = curve.Max.y;
            out->Count = curve.Count;

            for(uint i = 0; i < WCurve::MaxPoints; ++i)
            {
                out->Points[i].x = curve.Points[i].x;
                out->Points[i].y = curve.Points[i].y;
            }
        }

        void ParticleSystem_SetSizeOverLifeTime(uint64 entityId, ScriptCurve* curve)
        {
            if(curve == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<ParticleSystemComponent>()) return;

            auto& target = entity.get_mut<ParticleSystemComponent>().SizeOverLifeTime;
            target.Min = Vector2(curve->Min.x, curve->Min.y);
            target.Max = Vector2(curve->Max.x, curve->Max.y);
            target.Count = std::min(curve->Count, WCurve::MaxPoints);

            for(uint i = 0; i < WCurve::MaxPoints; ++i)
            {
                target.Points[i] = Vector2(curve->Points[i].x, curve->Points[i].y);
            }

            entity.modified<ParticleSystemComponent>();
        }
    }

    void RegisterParticleSystemCalls(Mono* runtime)
    {
        BIND(runtime, ParticleSystem_GetParticlesAmount);
        BIND(runtime, ParticleSystem_SetParticlesAmount);
        BIND(runtime, ParticleSystem_GetShape);
        BIND(runtime, ParticleSystem_SetShape);
        BIND(runtime, ParticleSystem_GetRadius);
        BIND(runtime, ParticleSystem_SetRadius);
        BIND(runtime, ParticleSystem_GetWorldSpace);
        BIND(runtime, ParticleSystem_SetWorldSpace);
        BIND(runtime, ParticleSystem_GetColor);
        BIND(runtime, ParticleSystem_SetColor);
        BIND(runtime, ParticleSystem_GetColorOverLifetime);
        BIND(runtime, ParticleSystem_SetColorOverLifetime);
        BIND(runtime, ParticleSystem_GetSizeMinMax);
        BIND(runtime, ParticleSystem_SetSizeMinMax);
        BIND(runtime, ParticleSystem_GetSpeedMinMax);
        BIND(runtime, ParticleSystem_SetSpeedMinMax);
        BIND(runtime, ParticleSystem_GetLifetimeMinMax);
        BIND(runtime, ParticleSystem_SetLifetimeMinMax);
        BIND(runtime, ParticleSystem_GetSizeOverLifeTime);
        BIND(runtime, ParticleSystem_SetSizeOverLifeTime);
    }
}
