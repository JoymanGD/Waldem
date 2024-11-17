#pragma once

#include "Waldem/Renderer/Light.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/World/Camera.h"
#include "Waldem/Renderer/Shader.h"
#include "ECS/include/ecs.h"
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
    protected:
        void Update(Waldem::SceneData* sceneData, float deltaTime) override;
        void Draw(Waldem::SceneData* sceneData, float deltaTime) override;
        void Initialize(Waldem::SceneData* sceneData) override;

    private:
        ecs::Manager ECSManager;
        Waldem::WArray<Waldem::ISystem*> UpdateSystems;
        Waldem::WArray<Waldem::ISystem*> DrawSystems;
    };
}
