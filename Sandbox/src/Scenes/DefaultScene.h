#pragma once

#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Model/Model.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/World/Camera.h"

namespace Sandbox
{
    class DefaultScene : public Waldem::Scene
    {
    protected:
        void Update(float deltaTime) override;
        void Draw(Waldem::SceneData* sceneData) override;
        void Initialize(Waldem::SceneData* sceneData) override;
    private:
        void CreateLights();
        
        std::unique_ptr<Waldem::Camera> MainCamera;
        Waldem::Model* TestModel;
        Waldem::PixelShader* TestPixelShader;
        Waldem::Transform TestModelTransform;
        std::vector<Waldem::Light> Lights;
    };
}
