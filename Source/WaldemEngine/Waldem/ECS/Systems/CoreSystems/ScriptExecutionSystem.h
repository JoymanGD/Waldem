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
            ECS::World.observer<ScriptComponent>("ScriptComponentChanged")
                .event(flecs::OnSet)
                .each([&](ECS::Entity entity, ScriptComponent& scriptComponent)
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

            ECS::World.observer<ScriptComponent>("ScriptComponentRemoved")
                .event(flecs::OnRemove)
                .each([&](ECS::Entity entity, ScriptComponent&)
                {
                    ScriptEngine::DestroyEntityInstance(entity.id());
                });

            ECS::World.system<ScriptComponent>("ScriptUpdateSystem")
                .kind(flecs::OnUpdate)
                .each([&](ECS::Entity entity, ScriptComponent& scriptComponent)
                {
                    if(!EditorSimulation::ShouldRunRuntimeSystems())
                    {
                        return;
                    }

                    ScriptEngine::OnUpdate(entity, scriptComponent, Time::DeltaTime);
                });

            ECS::World.system<ScriptComponent>("ScriptFixedUpdateSystem")
                .kind<ECS::OnFixedUpdate>()
                .each([&](ECS::Entity entity, ScriptComponent& scriptComponent)
                {
                    if(!EditorSimulation::ShouldRunRuntimeSystems())
                    {
                        return;
                    }

                    ScriptEngine::OnFixedUpdate(entity, scriptComponent, Time::FixedDeltaTime);
                });
        }
    };
}
