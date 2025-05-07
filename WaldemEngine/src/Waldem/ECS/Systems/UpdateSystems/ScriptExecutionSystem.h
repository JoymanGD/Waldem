#pragma once
#include "Waldem/ECS/Components/ScriptComponent.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API ScriptExecutionSystem : public ISystem
    {
    public:
        ScriptExecutionSystem(ECSManager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            for (auto [entity, scriptComponent] : Manager->EntitiesWith<ScriptComponent>())
            {
                //Update script logic
            }
        }

        void FixedUpdate(float fixedDeltaTime)
        {
            for (auto [entity, scriptComponent] : Manager->EntitiesWith<ScriptComponent>())
            {
                //FixedUpdate script logic
            }
        }

        void Draw(float deltaTime)
        {
            for (auto [entity, scriptComponent] : Manager->EntitiesWith<ScriptComponent>())
            {
                //Draw script logic
            }
        }
    };
}
