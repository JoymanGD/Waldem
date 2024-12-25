#pragma once

#include "Waldem/ECS/Systems/DebugSystem.h"
#include "Waldem/ECS/Systems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/FreeLookCameraSystem.h"
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
		GameLayer(Window* window, ecs::Manager* ecsManager) : Layer("GameLayer", window, ecsManager)
		{
			GameInputManager = {};
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
					GameInputManager.Broadcast(event);
				}
			}
		}

		void OnDrawUI(float deltaTime) override
		{        	
			CurrentScene->DrawUI(deltaTime);
		}

		void OpenScene(Scene* scene, SceneData* sceneData)
		{
			Renderer::Begin();
			scene->Initialize(sceneData, &GameInputManager, CoreECSManager);
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
        InputManager GameInputManager;
	};
}