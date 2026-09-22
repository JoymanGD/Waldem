#pragma once

#include "Waldem/Time.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Systems/System.h"
#include "GameSystems/PlayerControllerSystem.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/ProjectManagement/ProjectManager.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"
#include "Waldem/SceneManagement/SceneManager.h"

namespace Waldem
{
	class GameLayer : public Layer
	{
	private:
		bool SceneLoaded = false;
	public:
		GameLayer(CWindow* window) : Layer("GameLayer", window)
		{
			auto mainViewport = ViewportManager::GetMainViewport();
			ViewportManager::CreateViewport(GameViewport, "Game", Vector2(0, 0), Vector2(1920, 1080), Vector2(0, 1), mainViewport->GetFrameBuffer());
			InputManager = {};

			UpdateSystems.Add(new PlayerControllerSystem());

			ECS::World.observer<Camera>().without<EditorComponent>().yield_existing().event(flecs::OnAdd).each([&](flecs::entity entity, Camera& camera)
			{
				auto viewport = ViewportManager::GetGameViewport();
				viewport->LinkCamera(entity);
				
				camera.UpdateProjectionMatrix(camera.FieldOfView, viewport->Size.x / (float)viewport->Size.y, camera.NearPlane, camera.FarPlane);
				entity.modified<Camera>();
			});
		}

		void Initialize() override
		{
			if(auto viewport = ViewportManager::GetGameViewport())
			{
				viewport->IsFocused = true;
				viewport->IsMouseOver = true;
			}

			for (ISystem* system : UISystems)
			{
				system->Initialize();
			}
        	
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize();
			}
        	
			for (ISystem* system : DrawSystems)
			{
				system->Initialize();
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Initialize();
			}

			Initialized = true;
		}
        
		void Deinitialize() override
		{
			for (ISystem* system : UISystems)
			{
				system->Deinitialize();
			}
        	
			for (ISystem* system : UpdateSystems)
			{
				system->Deinitialize();
			}
        	
			for (ISystem* system : DrawSystems)
			{
				system->Deinitialize();
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Deinitialize();
			}

			Initialized = false;
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
					event.Handled = InputManager.Broadcast(event);
				}
			}
		}

		void Draw() override
		{
			auto viewport = ViewportManager::GetGameViewport();
			Renderer::Begin(viewport);
			ECS::RunDrawPipeline(Time::DeltaTime);
			Renderer::End(PRESENT);
		}

		void Begin() override {}
		void End() override {}
		void OnAttach() override {}
		void OnDetach() override{}
	};
}
