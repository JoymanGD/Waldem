#pragma once

#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/DrawSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/CollisionSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/PhysicsUpdateSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/PhysicsIntegrationSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/ScriptExecutionSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/SpatialAudioSystem.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/Renderer/Renderer.h"
#include <glm/gtc/integer.hpp>
#include "Waldem/SceneManagement/GameScene.h"

namespace Waldem
{
	class WALDEM_API GameLayer : public Layer
	{
	public:
		GameLayer(CWindow* window) : Layer("GameLayer", window)
		{
			InputManager = {};

			// DrawSystems.Add(new OceanSimulationSystem(ecsManager));
			// DrawSystems.Add(new HybridRenderingSystem());
			// DrawSystems.Add(new PostProcessSystem());
			DrawSystems.Add(new ScreenQuadSystem());
			
			UpdateSystems.Add(new SpatialAudioSystem());
			// PhysicsSystems.Add(new PhysicsIntegrationSystem());
			// PhysicsSystems.Add(new PhysicsUpdateSystem());
			// PhysicsSystems.Add(new CollisionSystem());

			ScriptSystem = new ScriptExecutionSystem();
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

			ScriptSystem->Initialize(&InputManager);

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

			ScriptSystem->Deinitialize();

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

		void OpenScene(GameScene* scene, SceneData* sceneData)
		{
			scene->Initialize();
			
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(&InputManager);
			}
			
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize(&InputManager);
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Initialize(&InputManager);
			}

			ScriptSystem->Initialize(&InputManager);
			
			// CurrentScene = scene;
		}

		// GameScene* GetCurrentScene() { return CurrentScene; }
        
		void Begin() override {}
		void End() override {}
		void OnAttach() override {}
		void OnDetach() override{}

	private:
		// GameScene* CurrentScene = nullptr;
		ScriptExecutionSystem* ScriptSystem = nullptr;
	};
}
