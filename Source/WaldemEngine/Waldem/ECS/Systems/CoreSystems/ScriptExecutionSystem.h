#pragma once
#include "Waldem/ECS/Components/ScriptComponent.h"
#include "Waldem/Editor/EditorSimulation.h"
#include "Waldem/ECS/Systems/CoreSystem.h"
#include "Waldem/Scripting/ScriptEngine.h"
#include "Waldem/Time.h"

namespace Waldem
{
    class WALDEM_API ScriptExecutionSystem : public ICoreSystem
    {
    public:
        ScriptExecutionSystem() {}

        void Initialize() override
        {
            ECS::World.observer<ScriptComponent>("ScriptComponentChanged").event(flecs::OnSet).each([&](ECS::Entity entity, ScriptComponent& scriptComponent)
            {
                if(EditorSimulation::ShouldRunRuntimeSystems())
                {
                    ScriptEngine::CreateEntityInstance(entity, scriptComponent);
                }
                else
                {
                    ScriptEngine::DestroyEntityInstance(entity.id());
                }
            });

            ECS::World.observer<ScriptComponent>("ScriptComponentRemoved").event(flecs::OnRemove).each([&](ECS::Entity entity, ScriptComponent&)
            {
                ScriptEngine::DestroyEntityInstance(entity.id());
            });

            ECS::World.system<ScriptComponent>("ScriptUpdateSystem").kind(flecs::OnUpdate).each([&](ECS::Entity entity, ScriptComponent& scriptComponent)
            {
                if(!EditorSimulation::ShouldRunRuntimeSystems())
                {
                    return;
                }

                ScriptEngine::OnUpdate(entity, scriptComponent, Time::DeltaTime);
            });

            ECS::World.system<ScriptComponent>("ScriptFixedUpdateSystem").kind<ECS::OnFixedUpdate>().each([&](ECS::Entity entity, ScriptComponent& scriptComponent)
            {
                if(!EditorSimulation::ShouldRunRuntimeSystems())
                {
                    return;
                }

                ScriptEngine::OnFixedUpdate(entity, scriptComponent, Time::FixedDeltaTime);
            });

            ECS::World.system<ScriptComponent>("ScriptLateUpdateSystem").kind<ECS::OnLateUpdate>().each([&](ECS::Entity entity, ScriptComponent& scriptComponent)
            {
                if(!EditorSimulation::ShouldRunRuntimeSystems())
                {
                    return;
                }

                ScriptEngine::OnLateUpdate(entity, scriptComponent, Time::DeltaTime);
            });

            ECS::World.observer<ScriptComponent, ColliderComponent>("ScriptColliderEvents").event(ECS::OnSet).each([&](ECS::Entity entity, ScriptComponent& scriptComponent, ColliderComponent& collider)
            {
                collider.OnCollisionEnter = [entity, scriptComponent](ECS::Entity other, const ContactsManifold& contacts)
                {
                    ScriptEngine::OnCollisionEvent(CollisionEventType::Enter, entity, other, scriptComponent, contacts);
                };
                
                collider.OnCollisionStay = [entity, scriptComponent](ECS::Entity other, const ContactsManifold& contacts)
                {
                    ScriptEngine::OnCollisionEvent(CollisionEventType::Stay, entity, other, scriptComponent, contacts);
                };
                
                collider.OnCollisionExit = [entity, scriptComponent](ECS::Entity other, const ContactsManifold& contacts)
                {
                    ScriptEngine::OnCollisionEvent(CollisionEventType::Exit, entity, other, scriptComponent, contacts);
                };
                
                collider.OnTriggerEnter = [entity, scriptComponent](ECS::Entity other)
                {
                    ScriptEngine::OnTriggerEvent(CollisionEventType::Enter, entity, other, scriptComponent);
                };
                
                collider.OnTriggerStay = [entity, scriptComponent](ECS::Entity other)
                {
                    ScriptEngine::OnTriggerEvent(CollisionEventType::Stay, entity, other, scriptComponent);
                };
                
                collider.OnTriggerExit = [entity, scriptComponent](ECS::Entity other)
                {
                    ScriptEngine::OnTriggerEvent(CollisionEventType::Exit, entity, other, scriptComponent);
                };
            });
        }
    };
}
