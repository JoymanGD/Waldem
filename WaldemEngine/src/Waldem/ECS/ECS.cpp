#include "wdpch.h"

#include "ECS.h"

#include "Components/AudioSource.h"
#include "Components/Light.h"
#include "Components/MeshComponent.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Waldem/Editor/AssetReference.h"

namespace Waldem
{
    namespace ECS
    {
        void RegisterTypes()
        {
            World.component<SceneEntity>("SceneEntity")
                .member<uint64>("ParentId")
                .member<float>("HierarchySlot")
                .member<bool>("VisibleInHierarchy");
            
            World.component<Vector3>("Vector3")
                .member<float>("x")
                .member<float>("y")
                .member<float>("z");
            
            World.component<WString>()
                .opaque(flecs::String)
                .serialize([](const flecs::serializer *s, const WString *data)
                {
                    const char *str = data->C_Str();
                    return s->value(flecs::String, &str);
                })
                .assign_string([](WString* data, const char *value)
                {
                    *data = value;
                });

            World.component<LightType>()
                .constant("Directional", LightType::Directional)
                .constant("Point", LightType::Point)
                .constant("Spot", LightType::Spot)
                .constant("Area", LightType::Area);
            
            World.component<AssetReference>()
                .opaque(flecs::String)
                .serialize([](const flecs::serializer *s, const AssetReference *data)
                {
                    WString refStr = data->Reference.string();
                    const char* cstr = refStr.C_Str();
                    s->member("Reference");
                    s->value(flecs::String, &cstr);
                    return 0;
                })
                .assign_string([](AssetReference* data, const char *value)
                {
                    data->Reference = Path(value);
                });
        }

        void RegisterComponents()
        {
            Transform::RegisterComponent(World);
            RigidBody::RegisterComponent(World);
            AudioSource::RegisterComponent(World);
            MeshComponent::RegisterComponent(World);
            Light::RegisterComponent(World);
        }

        flecs::entity CreateEntity(const WString& name, bool enabled)
        {
            flecs::entity entity = World.entity(name);

            if(enabled)
            {
                entity.enable();
            }
            else
            {
                entity.disable();
            }

            return entity;
        }

        flecs::entity CreateSceneEntity(const WString& name, bool enabled, bool visibleInHierarchy)
        {
            auto count = HierarchySlots.Allocate();
            flecs::entity entity = World.entity(name).set<SceneEntity>({
                .ParentId = 0,
                .HierarchySlot = (float)count,
                .VisibleInHierarchy = visibleInHierarchy,
            }).add<Transform>();

            if(enabled)
            {
                entity.enable();
            }
            else
            {
                entity.disable();
            }

            return entity;
        }

        flecs::entity CloneSceneEntity(flecs::entity entity)
        {
            auto newSlot = HierarchySlots.Allocate();
            
            auto sceneEntityComponent = entity.get<SceneEntity>();
            
            auto clone = entity.clone().set<SceneEntity>(
            {
                .ParentId = sceneEntityComponent->ParentId,
                .HierarchySlot = (float)newSlot,
                .VisibleInHierarchy = sceneEntityComponent->VisibleInHierarchy,
            });

            auto entityName = entity.name();
            WString cloneEntityName = std::string(entityName.c_str()) + "_Clone";
            
            FormatName(cloneEntityName);
            
            clone.set_name(cloneEntityName.C_Str());
            
            return clone;
        }
    }
}
