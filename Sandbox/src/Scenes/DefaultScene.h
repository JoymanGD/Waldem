#pragma once

#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/World/Camera.h"
#include "ecs.h"
#include "Waldem/ECS/Systems/System.h"

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
        void Initialize(Waldem::SceneData* sceneData, InputManager* inputManager) override;
        void DrawUI(float deltaTime) override;

    private:
        ecs::Manager ECSManager;
        Waldem::WArray<Waldem::ISystem*> UpdateSystems;
        Waldem::WArray<Waldem::ISystem*> DrawSystems;
    };
}