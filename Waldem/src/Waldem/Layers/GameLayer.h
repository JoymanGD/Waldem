#pragma once
#include "Waldem/ECS/Systems/DebugSystem.h"
#include "Waldem/ECS/Systems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Systems/ShadowmapRenderingSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
	struct MainCamera;

	class WALDEM_API GameLayer : public Layer
    {
    public:
        GameLayer(Window* window) : Layer("GameLayer", window)
        {
            CurrentInputManager = {};

        	auto cameraEntity = CoreECSManager.CreateEntity();
        	float aspectRatio = window->GetWidth() / window->GetHeight();
        	cameraEntity.Add<Transform>(Vector3(0, 0, 0));
        	cameraEntity.Add<Camera>(70.0f, aspectRatio, 0.001f, 1000.0f, 30.0f, 30.0f);
        	cameraEntity.Add<MainCamera>();

        	//do it after all entities set up
        	CoreECSManager.Refresh();
        	
        	CoreUpdateSystems.Add((ISystem*)new FreeLookCameraSystem(&CoreECSManager));
        	CoreUpdateSystems.Add((ISystem*)new DebugSystem(&CoreECSManager));

        	CoreDrawSystems.Add((ISystem*)new ShadowmapRenderingSystem(&CoreECSManager));
        	CoreDrawSystems.Add((ISystem*)new DeferredRenderingSystem(&CoreECSManager));
        	
        	SceneData sceneData = { window };
        	
        	for (ISystem* system : CoreUpdateSystems)
        	{
        		system->Initialize(&sceneData, &CurrentInputManager);
        	}
		
        	for (ISystem* system : CoreDrawSystems)
        	{
        		system->Initialize(&sceneData, &CurrentInputManager);
        	}
        }

        void OnUpdate(float deltaTime) override
        {
            CurrentScene->Update(deltaTime);

            CurrentScene->Draw(deltaTime);
        }

        void OnEvent(Event& event) override
        {
            auto eventType = event.GetEventType();

            switch (eventType)
            {
            case EventType::KeyPressed:
            case EventType::KeyReleased:
            case EventType::KeyTyped:
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
            case EventType::MouseMoved:
            case EventType::MouseScrolled:
                {
                    event.Handled = true;
                    CurrentInputManager.Broadcast(event);
                }
            }
        }

        void OnDrawUI(float deltaTime) override
        {
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
				    if (ImGui::MenuItem("New scene")) {}
				    if (ImGui::MenuItem("Open scene", "Ctrl+O")) {}
				    if (ImGui::MenuItem("Save scene")) {}

				    ImGui::Separator();
					
				    if (ImGui::BeginMenu("Options"))
				    {
				        ImGui::EndMenu();
				    }
					
				    ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}
        	
            CurrentScene->DrawUI(deltaTime);
        }

        void OpenScene(Scene* scene, SceneData* sceneData)
        {
		    Renderer::Begin();
            scene->Initialize(sceneData, &CurrentInputManager, &CoreECSManager);
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
    	
        ecs::Manager CoreECSManager;
    	WArray<ISystem*> CoreUpdateSystems;
    	WArray<ISystem*> CoreDrawSystems;
    };
}