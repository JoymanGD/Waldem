#pragma once

#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Renderer/Model/Model.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/World/Camera.h"

namespace Sandbox
{
    class DefaultScene : public Waldem::Scene
    {
    protected:
        void Update(float deltaTime) override;
        void Draw(Waldem::Renderer* renderer) override;
        void Initialize() override;
    private:
        void CreateLights();
        
        std::unique_ptr<Waldem::Camera> MainCamera;
        Waldem::Pipeline* RasterPipeline = nullptr;
        Waldem::Model* TestModel;
        Waldem::Transform TestModelTransform;
        std::vector<Waldem::Light> Lights;
    };
}
