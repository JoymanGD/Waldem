#pragma once

#include "Waldem/ECS/Systems/GameSystems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/GameSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/GameSystems/PostProcessSystem.h"
#include "Waldem/ECS/Systems/GameSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/GameSystems/CollisionSystem.h"
#include "Waldem/ECS/Systems/GameSystems/PhysicsSystem.h"
#include "Waldem/ECS/Systems/GameSystems/GBufferSystem.h"
#include "Waldem/ECS/Systems/GameSystems/RayTracingRadianceSystem.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/Renderer/Renderer.h"
#include <glm/gtc/integer.hpp>

namespace Waldem
{
	class WALDEM_API GameLayer : public Layer
	{
	public:
		GameLayer(Window* window, ecs::Manager* ecsManager, ResourceManager* resourceManager) : Layer("GameLayer", window, ecsManager, resourceManager)
		{
			GameInputManager = {};

			Vector2 resolution = Vector2(window->GetWidth(), window->GetHeight());

			// auto averageWorldPositionEntity = ecsManager->CreateEntity();
			// averageWorldPositionEntity.Add<Selected>();
			// averageWorldPositionEntity.Add<Transform>(Vector3(0, 0, 0));

			resourceManager->CreateRenderTarget("WorldPositionRT", resolution.x, resolution.y, TextureFormat::R32G32B32A32_FLOAT);
			resourceManager->CreateRenderTarget("NormalRT", resolution.x, resolution.y, TextureFormat::R16G16B16A16_FLOAT);
			resourceManager->CreateRenderTarget("ColorRT", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
			resourceManager->CreateRenderTarget("ORMRT", resolution.x, resolution.y, TextureFormat::R32G32B32A32_FLOAT);
			resourceManager->CreateRenderTarget("MeshIDRT", resolution.x, resolution.y, TextureFormat::R32_SINT);
			resourceManager->CreateRenderTarget("DepthRT", resolution.x, resolution.y, TextureFormat::D32_FLOAT);
			resourceManager->CreateRenderTarget("RadianceRT", resolution.x, resolution.y, TextureFormat::R32G32B32A32_FLOAT);

			// DrawSystems.Add((ISystem*)new OceanSimulationSystem(ecsManager));
			DrawSystems.Add((ISystem*)new GBufferSystem(ecsManager));
			DrawSystems.Add((ISystem*)new RayTracingRadianceSystem(ecsManager));
			DrawSystems.Add((ISystem*)new DeferredRenderingSystem(ecsManager));
			// DrawSystems.Add((ISystem*)new PostProcessSystem(ecsManager));
			DrawSystems.Add((ISystem*)new ScreenQuadSystem(ecsManager));
			// DrawSystems.Add((ISystem*)new CollisionSystem(ecsManager));
			// DrawSystems.Add((ISystem*)new PhysicsSystem(ecsManager));
		}

		void OnUpdate(float deltaTime) override
		{
			CurrentScene->Update(deltaTime);
			
			for (ISystem* system : DrawSystems)
			{
				system->Update(deltaTime);
			}

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
					event.Handled = GameInputManager.Broadcast(event);
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
			scene->Initialize(sceneData, &GameInputManager, CurrentECSManager, CurrentResourceManager);
			
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(sceneData, &GameInputManager, CurrentResourceManager);
			}
			
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
		WArray<ISystem*> DrawSystems;
	};
}
