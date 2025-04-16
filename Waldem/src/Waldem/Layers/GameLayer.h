#pragma once

#include "Waldem/ECS/Systems/GameSystems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/GameSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/GameSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/GameSystems/CollisionSystem.h"
#include "Waldem/ECS/Systems/GameSystems/PhysicsUpdateSystem.h"
#include "Waldem/ECS/Systems/GameSystems/PhysicsIntegrationSystem.h"
#include "Waldem/ECS/Systems/GameSystems/GBufferSystem.h"
#include "Waldem/ECS/Systems/GameSystems/RayTracingRadianceSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/Renderer/Renderer.h"
#include <glm/gtc/integer.hpp>
#include "Waldem/ECS/Systems/GameSystems/ScriptExecutionSystem.h"
#include "Waldem/ECS/Systems/GameSystems/SpatialAudioSystem.h"
#include "Waldem/SceneManagement/GameScene.h"

namespace Waldem
{
	class WALDEM_API GameLayer : public Layer
	{
	public:
		GameLayer(Window* window, ECSManager* ecsManager, ResourceManager* resourceManager) : Layer("GameLayer", window, ecsManager, resourceManager)
		{
			GameInputManager = {};

			Vector2 resolution = Vector2(window->GetWidth(), window->GetHeight());

			resourceManager->CreateRenderTarget("WorldPositionRT", resolution.x, resolution.y, TextureFormat::R32G32B32A32_FLOAT);
			resourceManager->CreateRenderTarget("NormalRT", resolution.x, resolution.y, TextureFormat::R16G16B16A16_FLOAT);
			resourceManager->CreateRenderTarget("ColorRT", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
			resourceManager->CreateRenderTarget("ORMRT", resolution.x, resolution.y, TextureFormat::R32G32B32A32_FLOAT);
			resourceManager->CreateRenderTarget("MeshIDRT", resolution.x, resolution.y, TextureFormat::R32_SINT);
			resourceManager->CreateRenderTarget("DepthRT", resolution.x, resolution.y, TextureFormat::D32_FLOAT);
			resourceManager->CreateRenderTarget("RadianceRT", resolution.x, resolution.y, TextureFormat::R32G32B32A32_FLOAT);

			DrawSystems.Add((ISystem*)new OceanSimulationSystem(ecsManager));
			DrawSystems.Add((ISystem*)new SpatialAudioSystem(ecsManager));
			DrawSystems.Add((ISystem*)new GBufferSystem(ecsManager));
			DrawSystems.Add((ISystem*)new RayTracingRadianceSystem(ecsManager));
			DrawSystems.Add((ISystem*)new DeferredRenderingSystem(ecsManager));
			// DrawSystems.Add((ISystem*)new PostProcessSystem(ecsManager));
			DrawSystems.Add((ISystem*)new ScreenQuadSystem(ecsManager));
			
			// PhysicsSystems.Add((ISystem*)new PhysicsIntegrationSystem(ecsManager));
			// PhysicsSystems.Add((ISystem*)new PhysicsUpdateSystem(ecsManager));
			// PhysicsSystems.Add((ISystem*)new CollisionSystem(ecsManager));

			ScriptSystem = new ScriptExecutionSystem(ecsManager);
		}

		void Initialize(SceneData* sceneData) override
		{
			Renderer::Begin();
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			}
			Renderer::End();
			
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			}

			ScriptSystem->Initialize(sceneData, &GameInputManager, CurrentResourceManager);

			Initialized = true;
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
					event.Handled = GameInputManager.Broadcast(event);
				}
			}
		}

		void OnDrawUI(float deltaTime) override
		{
			// CurrentScene->DrawUI(deltaTime);
		}

		void OpenScene(GameScene* scene, SceneData* sceneData)
		{
			Renderer::Begin();
			scene->Initialize(sceneData, &GameInputManager, CurrentECSManager, CurrentResourceManager);
			
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			}
			Renderer::End();
			
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			}
			
			for (ISystem* system : PhysicsSystems)
			{
				system->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			}

			ScriptSystem->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			
			// CurrentScene = scene;
		}

		// GameScene* GetCurrentScene() { return CurrentScene; }
        
		void Begin() override {}
		void End() override {}
		void OnAttach() override {}
		void OnDetach() override{}

	private:
		// GameScene* CurrentScene = nullptr;
		InputManager GameInputManager;
		WArray<ISystem*> DrawSystems;
		WArray<ISystem*> UpdateSystems;
		WArray<ISystem*> PhysicsSystems;
		ScriptExecutionSystem* ScriptSystem = nullptr;
	};
}
