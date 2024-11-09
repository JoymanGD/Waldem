#pragma once

#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Model/Model.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/World/Camera.h"
#include "Waldem/Renderer/Shader.h"

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
        void Update(float deltaTime) override;
        void Draw(Waldem::SceneData* sceneData) override;
        void Initialize(Waldem::SceneData* sceneData) override;
    private:
        void CreateLights();
        
        Waldem::Camera* MainCamera;
        Waldem::Model* TestModel;
        Waldem::PixelShader* TestPixelShader;
        Waldem::Transform TestModelTransform;
        std::vector<Waldem::Light> Lights;
        SceneConstantBuffer ConstantBuffer;
    };
}
