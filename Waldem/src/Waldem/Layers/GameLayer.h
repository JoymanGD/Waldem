#pragma once
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API GameLayer : public Layer
    {
    public:
        GameLayer() : Layer("Example")
        {
            CurrentInputManager = {};
        }

        void OnUpdate(float deltaTime) override
        {
            CurrentScene->Update(deltaTime);

            CurrentScene->Draw(deltaTime);
        }

        void OnEvent(Event& event) override
        {
            CurrentInputManager.Broadcast(event);
        }

        void OnDrawUI(float deltaTime) override
        {
            CurrentScene->DrawUI(deltaTime);
        }

        void OpenScene(Scene* scene, SceneData* sceneData)
        {
		    Renderer::Begin();
            scene->Initialize(sceneData, &CurrentInputManager);
		    Renderer::End();
            
            CurrentScene = scene;
        }

        Scene* GetCurrentScene() { return CurrentScene; }
        
        void Begin() override {}
        void End() override {}
        void OnAttach() override {}
        void OnDetach() override{}

    private:
        Scene* CurrentScene = nullptr;
        InputManager CurrentInputManager;
    };
}