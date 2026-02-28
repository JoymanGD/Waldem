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
#include "Systems/CoreSystems/CollisionSystem.h"
#include "Systems/CoreSystems/HybridRenderingSystem.h"
#include "Systems/CoreSystems/ParticleSystem.h"
#include "Systems\CoreSystems\PhysicsAccumulationSystem.h"
#include "Systems\CoreSystems\PhysicsIntegrationSystem.h"
#include "Systems/CoreSystems/PostProcessSystem.h"
#include "Systems/CoreSystems/ScreenQuadSystem.h"
#include "Systems/CoreSystems/SpatialAudioSystem.h"
#include "Waldem/Editor/AssetReference.h"
#include "Waldem/Editor/AssetReference/AudioClipReference.h"
#include "Waldem/Editor/AssetReference/MeshReference.h"
#include "Waldem/Editor/AssetReference/TextureReference.h"
#include "Waldem/Editor/AssetReference/MaterialReference.h"
#include "Waldem/Utils/ECSUtils.h"
#include <cmath>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Waldem
{
    namespace ECS
    {
        WALDEM_API flecs::world World{};
        WALDEM_API EntityT UpdatePipeline{};
        WALDEM_API EntityT FixedUpdatePipeline{};
        WALDEM_API EntityT DrawPipeline{};
        WALDEM_API EntityT GUIPipeline{};
        WALDEM_API WMap<WString, Entity> RegisteredComponents{};
        WALDEM_API FreeList HierarchySlots{};

        namespace
        {
            std::unordered_map<EntityT, Matrix4> LocalTransformMatrices{};
            std::unique_ptr<flecs::query<Transform, const Transform>> HierarchyTransformQuery{};
            bool IsParentingInternalUpdate = false;
            bool IsTransformHierarchyUpdate = false;

            ecs_entity_t GetParentIdRaw(ecs_entity_t entityId)
            {
                if(entityId == 0)
                {
                    return 0;
                }

                return ecs_get_target(World.c_ptr(), entityId, EcsChildOf, 0);
            }

            Entity GetParentEntityInternal(Entity entity)
            {
                const ecs_entity_t parentId = GetParentIdRaw(entity.id());
                if(parentId == 0)
                {
                    return {};
                }

                return World.entity(parentId);
            }

            bool IsDescendantOf(ecs_entity_t entityId, ecs_entity_t possibleAncestorId)
            {
                if(entityId == 0 || possibleAncestorId == 0)
                {
                    return false;
                }

                ecs_entity_t current = GetParentIdRaw(entityId);
                while(current != 0)
                {
                    if(current == possibleAncestorId)
                    {
                        return true;
                    }

                    current = GetParentIdRaw(current);
                }

                return false;
            }

            void CacheLocalMatrixFromWorld(Entity entity)
            {
                if(!entity.is_alive() || !entity.has<Transform>())
                {
                    return;
                }

                const auto& transform = entity.get<Transform>();
                Matrix4 localMatrix = transform.Matrix;

                Entity parent = GetParentEntityInternal(entity);
                if(parent.is_alive() && parent.has<Transform>())
                {
                    localMatrix = inverse(parent.get<Transform>().Matrix) * transform.Matrix;
                }

                LocalTransformMatrices[entity.id()] = localMatrix;
            }

            void RefreshHierarchyDepths()
            {
                auto query = World.query_builder<SceneEntity>().build();

                const bool previousInternalFlag = IsParentingInternalUpdate;
                IsParentingInternalUpdate = true;
                query.each([&](flecs::entity entity, SceneEntity& sceneEntity)
                {
                    uint32 depth = 0;
                    ecs_entity_t parentId = GetParentIdRaw(entity.id());
                    while(parentId != 0)
                    {
                        depth++;
                        parentId = GetParentIdRaw(parentId);
                    }

                    const float computedDepth = static_cast<float>(depth);
                    if(sceneEntity.HierarchyDepth != computedDepth)
                    {
                        sceneEntity.HierarchyDepth = computedDepth;
                        entity.modified<SceneEntity>();
                    }
                });
                IsParentingInternalUpdate = previousInternalFlag;
            }

            void ApplyParent(Entity child, Entity parent, bool keepWorldTransform)
            {
                if(!child.is_alive() || !child.has<SceneEntity>())
                {
                    return;
                }

                if(parent.is_alive() && (!parent.has<SceneEntity>() || parent.id() == child.id() || IsDescendantOf(parent.id(), child.id())))
                {
                    return;
                }

                Matrix4 childWorldMatrix(1.0f);
                const bool hasTransform = child.has<Transform>();
                if(hasTransform)
                {
                    childWorldMatrix = child.get<Transform>().Matrix;
                }

                const bool previousInternalFlag = IsParentingInternalUpdate;
                IsParentingInternalUpdate = true;
                ecs_remove_pair(World.c_ptr(), child.id(), EcsChildOf, EcsWildcard);
                if(parent.is_alive())
                {
                    ecs_add_pair(World.c_ptr(), child.id(), EcsChildOf, parent.id());
                }

                auto& sceneEntity = child.get_mut<SceneEntity>();
                sceneEntity.ParentId = parent.is_alive() ? (uint64)parent.id() : 0;
                child.modified<SceneEntity>();
                IsParentingInternalUpdate = previousInternalFlag;

                if(hasTransform)
                {
                    Matrix4 localMatrix = childWorldMatrix;
                    if(parent.is_alive() && parent.has<Transform>())
                    {
                        localMatrix = inverse(parent.get<Transform>().Matrix) * childWorldMatrix;
                    }

                    if(keepWorldTransform)
                    {
                        auto& childTransform = child.get_mut<Transform>();
                        childTransform.Matrix = childWorldMatrix;
                        childTransform.DecompileMatrix();
                        LocalTransformMatrices[child.id()] = localMatrix;
                        child.modified<Transform>();
                    }
                    else
                    {
                        CacheLocalMatrixFromWorld(child);
                    }
                }

                RefreshHierarchyDepths();
            }

            bool MatricesNear(const Matrix4& lhs, const Matrix4& rhs, float epsilon = 0.00001f)
            {
                const float* leftPtr = glm::value_ptr(lhs);
                const float* rightPtr = glm::value_ptr(rhs);
                for(int i = 0; i < 16; i++)
                {
                    if(fabsf(leftPtr[i] - rightPtr[i]) > epsilon)
                    {
                        return false;
                    }
                }

                return true;
            }
        }
        
        void Core::Initialize()
        {
            World = flecs::world();
            UpdatePipeline = World.pipeline().with(flecs::System).with(flecs::OnUpdate).build();
            FixedUpdatePipeline = World.pipeline().with(flecs::System).with<OnFixedUpdate>().build();
            DrawPipeline = World.pipeline().with(flecs::System).with<OnDraw>().build();
            GUIPipeline = World.pipeline().with(flecs::System).with<OnGUI>().build();
            RegisteredComponents = {};
            HierarchySlots = {};
            LocalTransformMatrices = {};
            
            RegisterTypes();
            RegisterAllComponents();

            //TODO: move to a separate system file
            World.observer<SceneEntity>().event(flecs::OnRemove).each([&](flecs::entity entity, SceneEntity& sceneEntity)
            {
                HierarchySlots.Free((int)sceneEntity.HierarchySlot);
                LocalTransformMatrices.erase(entity.id());
            });
            
            World.observer<SceneEntity>().event(flecs::OnSet).each([&](flecs::entity entity, SceneEntity& sceneEntity)
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

                if(IsParentingInternalUpdate)
                {
                    return;
                }

                const Entity currentParent = GetParentEntityInternal(entity);
                const Entity requestedParent = sceneEntity.ParentId == 0 ? Entity{} : World.entity((EntityT)sceneEntity.ParentId);

                if(sceneEntity.ParentId != 0 && !requestedParent.is_alive())
                {
                    return;
                }

                if(requestedParent.is_alive() && (requestedParent.id() == entity.id() || IsDescendantOf(requestedParent.id(), entity.id())))
                {
                    ApplyParent(entity, {}, true);
                    return;
                }

                const EntityT requestedParentId = requestedParent.is_alive() ? requestedParent.id() : 0;
                const EntityT currentParentId = currentParent.is_alive() ? currentParent.id() : 0;
                if(currentParentId != requestedParentId)
                {
                    ApplyParent(entity, requestedParent, true);
                }
                else if(entity.has<Transform>() && LocalTransformMatrices.find(entity.id()) == LocalTransformMatrices.end())
                {
                    CacheLocalMatrixFromWorld(entity);
                }
            });

            World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform&)
            {
                if(IsTransformHierarchyUpdate)
                {
                    return;
                }

                CacheLocalMatrixFromWorld(entity);
            });

            World.observer<Transform>().event(flecs::OnRemove).each([&](flecs::entity entity, Transform&)
            {
                LocalTransformMatrices.erase(entity.id());
            });

            HierarchyTransformQuery = std::make_unique<flecs::query<Transform, const Transform>>(World.query_builder<Transform, const Transform>().term_at(1).parent().cascade().build());

            World.system("HierarchyTransformPropagation").kind(flecs::OnUpdate).run([&](flecs::iter&)
            {
                if(!HierarchyTransformQuery)
                {
                    return;
                }

                HierarchyTransformQuery->each([&](flecs::entity entity, Transform& transform, const Transform& parentTransform)
                {
                    auto localTransformIt = LocalTransformMatrices.find(entity.id());
                    Matrix4 localMatrix = localTransformIt == LocalTransformMatrices.end()
                        ? inverse(parentTransform.Matrix) * transform.Matrix
                        : localTransformIt->second;

                    const Matrix4 targetWorldMatrix = parentTransform.Matrix * localMatrix;
                    if(MatricesNear(transform.Matrix, targetWorldMatrix))
                    {
                        return;
                    }

                    const bool previousTransformFlag = IsTransformHierarchyUpdate;
                    IsTransformHierarchyUpdate = true;
                    transform.Matrix = targetWorldMatrix;
                    transform.DecompileMatrix();
                    entity.modified<Transform>();
                    IsTransformHierarchyUpdate = previousTransformFlag;
                });
            });
            
            // Systems.Add(OceanSimulationSystem());
            Systems.Add(new PhysicsAccumulationSystem());
            Systems.Add(new PhysicsIntegrationSystem());
            Systems.Add(new CollisionSystem());
            Systems.Add(new SpatialAudioSystem());
            Systems.Add(new AnimationSystem());
            Systems.Add(new HybridRenderingSystem());
            Systems.Add(new ParticleSystem());
            Systems.Add(new PostProcessSystem());
            Systems.Add(new ScreenQuadSystem());

            InitializeSystems();
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
                .member("x", &Vector3::x)
                .member("y", &Vector3::y)
                .member("z", &Vector3::z);
            
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
            
            World.component<AssetReference>()
                .opaque(flecs::String)
                .serialize([](const flecs::serializer *s, const AssetReference *data)
                {
                    WString refStr = data->Reference.string();
                    const char* cstr = refStr.C_Str();
                    s->value(flecs::String, &cstr);
                    return 0;
                })
                .assign_string([](AssetReference* data, const char *value)
                {
                    data->Reference = Path(value);
                });

            World.component<MeshReference>()
                .opaque(flecs::String)
                .serialize([](const flecs::serializer *s, const MeshReference *data)
                {
                    WString refStr = data->Reference.string();
                    const char* cstr = refStr.C_Str();
                    s->value(flecs::String, &cstr);
                    return 0;
                })
                .assign_string([](MeshReference* data, const char *value)
                {
                    data->Reference = Path(value);
                });

            World.component<MaterialReference>()
                .opaque(flecs::String)
                .serialize([](const flecs::serializer *s, const MaterialReference *data)
                {
                    WString refStr = data->Reference.string();
                    const char* cstr = refStr.C_Str();
                    s->value(flecs::String, &cstr);
                    return 0;
                })
                .assign_string([](MaterialReference* data, const char *value)
                {
                    data->Reference = Path(value);
                });

            World.component<TextureReference>()
                .opaque(flecs::String)
                .serialize([](const flecs::serializer *s, const TextureReference *data)
                {
                    WString refStr = data->Reference.string();
                    const char* cstr = refStr.C_Str();
                    s->value(flecs::String, &cstr);
                    return 0;
                })
                .assign_string([](TextureReference* data, const char *value)
                {
                    data->Reference = Path(value);
                });

            World.component<AudioClipReference>()
                .opaque(flecs::String)
                .serialize([](const flecs::serializer *s, const AudioClipReference *data)
                {
                    WString refStr = data->Reference.string();
                    const char* cstr = refStr.C_Str();
                    s->value(flecs::String, &cstr);
                    return 0;
                })
                .assign_string([](AudioClipReference* data, const char *value)
                {
                    data->Reference = Path(value);
                });
        }

        void Core::RegisterAllComponents()
        {
            for (auto fn : ComponentsRegistry)
            {
                fn(World);
            }
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
            CacheLocalMatrixFromWorld(entity);

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
            
            auto& sceneEntityComponent = entity.get<SceneEntity>();
            
            auto clone = entity.clone().set<SceneEntity>(
            {
                .ParentId = sceneEntityComponent.ParentId,
                .HierarchySlot = (float)newSlot,
                .VisibleInHierarchy = sceneEntityComponent.VisibleInHierarchy,
            });
            CacheLocalMatrixFromWorld(clone);

            auto entityName = entity.name();
            WString cloneEntityName = std::string(entityName.c_str()) + "_Clone";
            
            FormatName(cloneEntityName);
            
            clone.set_name(cloneEntityName.C_Str());
            
            return clone;
        }

        void SetParent(Entity child, Entity parent, bool keepWorldTransform)
        {
            ApplyParent(child, parent, keepWorldTransform);
        }

        void ClearParent(Entity child, bool keepWorldTransform)
        {
            ApplyParent(child, {}, keepWorldTransform);
        }

        Entity GetParent(Entity child)
        {
            if(!child.is_alive())
            {
                return {};
            }

            return GetParentEntityInternal(child);
        }

        void RebuildParentRelations()
        {
            auto query = World.query_builder<SceneEntity>().build();
            std::vector<std::pair<EntityT, EntityT>> parentingRequests;

            query.each([&](flecs::entity entity, const SceneEntity& sceneEntity)
            {
                parentingRequests.emplace_back(entity.id(), (EntityT)sceneEntity.ParentId);
            });

            for(const auto& [childId, parentId] : parentingRequests)
            {
                Entity child = World.entity(childId);
                if(!child.is_alive())
                {
                    continue;
                }

                if(parentId == 0)
                {
                    ApplyParent(child, {}, true);
                    continue;
                }

                Entity parent = World.entity(parentId);
                if(!parent.is_alive() || parent.id() == child.id() || IsDescendantOf(parent.id(), child.id()))
                {
                    ApplyParent(child, {}, true);
                    continue;
                }

                ApplyParent(child, parent, true);
            }
        }
    }
}
