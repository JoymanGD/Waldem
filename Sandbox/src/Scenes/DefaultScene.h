#pragma once

#include "Waldem/Renderer/Camera.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/SceneManagement/Scene.h"

namespace Sandbox
{
    class DefaultScene : public Waldem::Scene
    {
    protected:
        void Update(float deltaTime) override;
        void Draw() override;
        void Initialize() override;
    private:
        std::unique_ptr<Waldem::Mesh> QuadMesh;
        std::unique_ptr<Waldem::Mesh> TrisMesh;
        std::unique_ptr<Waldem::Camera> MainCamera;
    };
}