#pragma once

#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/World/Camera.h"
#include "ecs.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"

namespace Sandbox
{
    struct SceneConstantBuffer
    {
        Waldem::Matrix4 ViewProjectionMatrix;
        Waldem::Matrix4 ModelMatrix;
        uint32_t LightCount;
    };
    
    class DefaultScene : public Waldem::Scene
    {
    public:
        void Update(float deltaTime) override;
        void Draw(float deltaTime) override;
        void Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, ecs::Manager* ecsManager) override;
        void DrawUI(float deltaTime) override;

    private:
        Waldem::WArray<Waldem::ISystem*> UpdateSystems;
        Waldem::WArray<Waldem::ISystem*> DrawSystems; 
    };
}
