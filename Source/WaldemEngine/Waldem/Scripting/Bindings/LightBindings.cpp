#include "wdpch.h"
#include "LightBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/ECS/Components/Light.h"

namespace Waldem::Bindings
{
    namespace
    {
        struct ScriptVector3 { float x = 0.0f, y = 0.0f, z = 0.0f; };

        void Light_GetColor(uint64_t entityId, ScriptVector3* out)
        {
            if(out == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Light>()) { *out = {}; return; }
            const auto& l = entity.get<Light>();
            out->x = l.Color.x; out->y = l.Color.y; out->z = l.Color.z;
        }

        void Light_SetColor(uint64_t entityId, ScriptVector3* color)
        {
            if(color == nullptr) return;
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Light>()) return;
            auto& l = entity.get_mut<Light>();
            l.Color = Vector3(color->x, color->y, color->z);
            entity.modified<Light>();
        }

        float Light_GetIntensity(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Light>()) return 0.0f;
            return entity.get<Light>().Intensity;
        }

        void Light_SetIntensity(uint64_t entityId, float intensity)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Light>()) return;
            entity.get_mut<Light>().Intensity = intensity;
            entity.modified<Light>();
        }

        float Light_GetRadius(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Light>()) return 0.0f;
            return entity.get<Light>().Radius;
        }

        void Light_SetRadius(uint64_t entityId, float radius)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<Light>()) return;
            entity.get_mut<Light>().Radius = radius;
            entity.modified<Light>();
        }
    }

    void RegisterLightCalls(Mono* runtime)
    {
        BIND(runtime, Light_GetColor);
        BIND(runtime, Light_SetColor);
        BIND(runtime, Light_GetIntensity);
        BIND(runtime, Light_SetIntensity);
        BIND(runtime, Light_GetRadius);
        BIND(runtime, Light_SetRadius);
    }
}
