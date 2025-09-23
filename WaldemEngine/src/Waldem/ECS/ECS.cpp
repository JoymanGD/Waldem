#include "wdpch.h"

#include "ECS.h"

#include "Components/AnimationListener.h"
#include "Components/AudioSource.h"
#include "Components/Light.h"
#include "Components/MeshComponent.h"
#include "Components/ParticleSystemComponent.h"
#include "Components/PlayerController.h"
#include "Components/RigidBody.h"
#include "Components/Selected.h"
#include "Components/Sky.h"
#include "Components/Sprite.h"
#include "Components/Transform.h"
#include "glm/gtc/type_ptr.hpp"
#include "Systems/CoreSystems/AnimationSystem.h"
#include "Systems/CoreSystems/EntitySelectionSystem.h"
#include "Systems/CoreSystems/HybridRenderingSystem.h"
#include "Systems/CoreSystems/ParticleSystem.h"
#include "Systems/CoreSystems/PostProcessSystem.h"
#include "Systems/CoreSystems/ScreenQuadSystem.h"
#include "Systems/CoreSystems/SpatialAudioSystem.h"
#include "Systems/EditorSystems/GizmosRenderingSystem.h"
#include "Systems/EditorSystems/WorldGridRenderingSystem.h"
#include "Waldem/Editor/AssetReference.h"

namespace Waldem
{
    namespace ECS
    {
        void Core::Initialize()
        {
            World = flecs::world();

            flecs::OnFixedUpdate = World.entity("OnFixedUpdate");
            flecs::OnDraw = World.entity("OnDraw");
            flecs::OnGUI = World.entity("OnGUI");
            
            UpdatePipeline = World.pipeline().with(flecs::System).with(flecs::OnUpdate).build();
            FixedUpdatePipeline = World.pipeline().with(flecs::System).with(flecs::OnFixedUpdate).build();
            DrawPipeline = World.pipeline().with(flecs::System).with(flecs::OnDraw).build();
            GUIPipeline = World.pipeline().with(flecs::System).with(flecs::OnGUI).build();

            RegisterTypes();
            RegisterComponents();

            //TODO: move to a separate system file
            World.observer<SceneEntity>().event(flecs::OnRemove).each([&](SceneEntity& sceneEntity)
            {
                HierarchySlots.Free((int)sceneEntity.HierarchySlot);
            });
            
            World.observer<SceneEntity>().event(flecs::OnSet).each([&](SceneEntity& sceneEntity)
            {
                bool slotIsValid = sceneEntity.HierarchySlot >= 0.f;
                bool slotIsAllocated = slotIsValid && HierarchySlots.IsAllocated((int)sceneEntity.HierarchySlot);

                if(!slotIsAllocated)
                {
                    if(slotIsValid)
                    {
                        HierarchySlots.MarkAsAllocated((int)sceneEntity.HierarchySlot);
                    }
                    else
                    {
                        sceneEntity.HierarchySlot = (float)HierarchySlots.Allocate();
                    }
                }
            });
            
            // Systems.Add(OceanSimulationSystem());
            // Systems.Add(PhysicsIntegrationSystem());
            // Systems.Add(PhysicsUpdateSystem());
            // Systems.Add(CollisionSystem());
            Systems.Add(new SpatialAudioSystem());
            Systems.Add(new AnimationSystem());
            Systems.Add(new HybridRenderingSystem());
            Systems.Add(new WorldGridRenderingSystem());
            Systems.Add(new GizmosRenderingSystem());
            Systems.Add(new ParticleSystem());
            Systems.Add(new PostProcessSystem());
            Systems.Add(new ScreenQuadSystem());
        }

        void Core::InitializeSystems()
        {
            for (auto system : Systems)
            {
                system->Initialize();
            }
        }

        void Core::RegisterTypes()
        {
            World.component<SceneEntity>("SceneEntity")
                .member<uint64>("ParentId")
                .member<float>("HierarchyDepth")
                .member<float>("HierarchySlot")
                .member<bool>("VisibleInHierarchy");
            
            World.component<Vector2>("Vector2")
                .member<float>("x")
                .member<float>("y");
            
            World.component<Vector3>("Vector3")
                .member<float>("x")
                .member<float>("y")
                .member<float>("z");
            
            World.component<Vector4>("Vector4")
                .member<float>("x")
                .member<float>("y")
                .member<float>("z")
                .member<float>("w");

            World.component<Quaternion>("Quaternion")
                .opaque(
                    World.component()
                        .member<float>("x")
                        .member<float>("y")
                        .member<float>("z")
                        .member<float>("w")
                )
                .serialize([](const flecs::serializer *s, const Quaternion *q) {
                    s->member("x"); s->value(q->x);
                    s->member("y"); s->value(q->y);
                    s->member("z"); s->value(q->z);
                    s->member("w"); s->value(q->w);
                    return 0;
                })
                .ensure_member([](Quaternion *dst, const char *member) -> void* {
                    if (!strcmp(member, "x")) return &dst->x;
                    if (!strcmp(member, "y")) return &dst->y;
                    if (!strcmp(member, "z")) return &dst->z;
                    if (!strcmp(member, "w")) return &dst->w;
                    return nullptr;
                });

            World.component<Matrix4>("Matrix4")
                .opaque(
                    World.component()
                        .member<float>("m00").member<float>("m01").member<float>("m02").member<float>("m03")
                        .member<float>("m10").member<float>("m11").member<float>("m12").member<float>("m13")
                        .member<float>("m20").member<float>("m21").member<float>("m22").member<float>("m23")
                        .member<float>("m30").member<float>("m31").member<float>("m32").member<float>("m33")
                )
                .serialize([](const flecs::serializer *s, const Matrix4 *m) {
                    const float *ptr = glm::value_ptr(*m);
                    for (int i = 0; i < 16; i++) {
                        char name[8];
                        sprintf(name, "m%d%d", i/4, i%4);
                        s->member(name);
                        s->value(ptr[i]);
                    }
                    return 0;
                })
                .ensure_member([](Matrix4 *dst, const char *member) -> void* {
                    float *ptr = glm::value_ptr(*dst);
                    for (int row = 0; row < 4; row++) {
                        for (int col = 0; col < 4; col++) {
                            char name[8];
                            sprintf(name, "m%d%d", row, col);
                            if (!strcmp(member, name)) {
                                return &ptr[row * 4 + col];
                            }
                        }
                    }
                    return nullptr;
                });
            
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

        void Core::RegisterComponents()
        {
            Transform::RegisterComponent(World);
            Camera::RegisterComponent(World);
            RigidBody::RegisterComponent(World);
            AudioSource::RegisterComponent(World);
            MeshComponent::RegisterComponent(World);
            Light::RegisterComponent(World);
            Sky::RegisterComponent(World);
            Sprite::RegisterComponent(World);
            AnimationListener::RegisterComponent(World);
            Selected::RegisterComponent(World);
            ParticleSystemComponent::RegisterComponent(World);
            PlayerController::RegisterComponent(World);
            BloomPostProcess::RegisterComponent(World);
        }

        Entity CreateEntity(const WString& name, bool enabled)
        {
            Entity entity = World.entity(name);

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

        Entity CreateSceneEntity(const WString& name, bool enabled, bool visibleInHierarchy)
        {
            auto slot = HierarchySlots.Allocate();
            
            Entity entity = World.entity().set<SceneEntity>({
                .ParentId = 0,
                .HierarchySlot = (float)slot,
                .VisibleInHierarchy = visibleInHierarchy,
            }).add<Transform>();

            WString formattedName = name;
            FormatName(formattedName);
            
            entity.set_name(formattedName.C_Str());

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

        Entity CloneSceneEntity(Entity entity)
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
