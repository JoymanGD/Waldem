#pragma once

#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/World/Camera.h"
#include "ecs.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Renderer/Resources/ResourceManager.h"

namespace Sandbox
{
    class DefaultScene : public Waldem::Scene
    {
    public:
        void Update(float deltaTime) override;
        void Draw(float deltaTime) override;
        void Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, ecs::Manager* ecsManager, Waldem::ResourceManager* resourceManager) override;
        void DrawUI(float deltaTime) override;

    private:
        Waldem::WArray<Waldem::ISystem*> UpdateSystems;
        Waldem::WArray<Waldem::ISystem*> DrawSystems;
    };
}
