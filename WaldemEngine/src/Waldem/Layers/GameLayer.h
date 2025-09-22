#pragma once

#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/GameSystems/PlayerControllerSystem.h"
#include "Waldem/ECS/Systems/GameSystems/ScriptExecutionSystem.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"

namespace Waldem
{
	class WALDEM_API GameLayer : public Layer
	{
	public:
		GameLayer(CWindow* window) : Layer("GameLayer", window)
		{
			InputManager = {};

			UpdateSystems.Add(new PlayerControllerSystem());

			//Should be added last
			UpdateSystems.Add(new ScriptExecutionSystem());

			ECS::World.observer<Camera>().without<EditorComponent>().event(flecs::OnAdd).each([&](flecs::entity entity, Camera& camera)
			{
				auto viewport = ViewportManager::GetGameViewport();
				viewport->LinkCamera(entity);
				
				camera.UpdateProjectionMatrix(camera.FieldOfView, viewport->Size.x / (float)viewport->Size.y, camera.NearPlane, camera.FarPlane);
				entity.modified<Camera>(); 
			});

			ViewportManager::GetGameViewport()->SubscribeOnResize([this](Vector2 size)
			{
				OnResize(size);
			});
		}

		void OnResize(Vector2 size)
		{
			ECS::Entity linkedCamera;
			if(ViewportManager::GetGameViewport()->TryGetLinkedCamera(linkedCamera))
			{
				auto camera = linkedCamera.get_mut<Camera>();
				camera->UpdateProjectionMatrix(camera->FieldOfView, size.x/size.y, camera->NearPlane, camera->FarPlane);
				linkedCamera.modified<Camera>(); 
			}
		}

		void Initialize() override
		{
			for (ISystem* system : UISystems)
			{
				system->Initialize(&InputManager);
			}
        	
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize(&InputManager);
			}
        	
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(&InputManager);
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Initialize(&InputManager);
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

		void Begin() override {}
		void End() override {}
		void OnAttach() override {}
		void OnDetach() override{}
	};
}
