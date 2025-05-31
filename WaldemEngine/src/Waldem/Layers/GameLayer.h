#pragma once

#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/DrawSystems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/GBufferSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/RayTracingRadianceSystem.h"
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
		GameLayer(CWindow* window, ECSManager* ecsManager, ResourceManager* resourceManager) : Layer("GameLayer", window, ecsManager, resourceManager)
		{
			InputManager = {};

			// DrawSystems.Add(new OceanSimulationSystem(ecsManager));
			DrawSystems.Add(new GBufferSystem(ecsManager));
			DrawSystems.Add(new RayTracingRadianceSystem(ecsManager));
			DrawSystems.Add(new DeferredRenderingSystem(ecsManager));
			// DrawSystems.Add(new PostProcessSystem(ecsManager));
			DrawSystems.Add(new ScreenQuadSystem(ecsManager));
			
			UpdateSystems.Add(new SpatialAudioSystem(ecsManager));
			// PhysicsSystems.Add(new PhysicsIntegrationSystem(ecsManager));
			// PhysicsSystems.Add(new PhysicsUpdateSystem(ecsManager));
			// PhysicsSystems.Add(new CollisionSystem(ecsManager));

			ScriptSystem = new ScriptExecutionSystem(ecsManager);
		}

		void Initialize() override
		{
			for (ISystem* system : UISystems)
			{
				system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			}
        	
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			}
        	
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			}

			ScriptSystem->Initialize(&InputManager, CurrentResourceManager, &ContentManager);

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

		void OnUpdate(float deltaTime) override
		{
			for (ISystem* system : UpdateSystems)
			{
				system->Update(deltaTime);
			}
			
			// CurrentScene->Update(deltaTime);

			ScriptSystem->Update(deltaTime);
		}

		void OnFixedUpdate(float fixedDeltaTime) override
		{
			for (ISystem* system : PhysicsSystems)
			{
				system->Update(fixedDeltaTime);
			}

			// CurrentScene->FixedUpdate(fixedDeltaTime);

			ScriptSystem->FixedUpdate(fixedDeltaTime);
		}

		void OnDraw(float deltaTime) override
		{
			for (ISystem* system : DrawSystems)
			{
				system->Update(deltaTime);
			}

			// CurrentScene->Draw(deltaTime);

			ScriptSystem->Draw(deltaTime);
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

		void OnDrawUI(float deltaTime) override
		{
			// CurrentScene->DrawUI(deltaTime);
		}

		void OpenScene(GameScene* scene, SceneData* sceneData)
		{
			scene->Initialize(&InputManager, CurrentECSManager, CurrentResourceManager);
			
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			}
			
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			}

			ScriptSystem->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
			
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
