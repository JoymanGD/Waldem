#include <wdpch.h>
#include "DefaultScene.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/ECS/Systems/DebugSystem.h"
#include "Waldem/ECS/Systems/DeferredRenderingSystem.h"
#include "Waldem/Import/ModelImporter.h"
#include "imgui.h"
#include "Waldem/ECS/Systems/ShadowmapRenderingSystem.h"

namespace Sandbox
{
	void DefaultScene::Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, ecs::Manager* ecsManager)
	{
		Waldem::ModelImporter importer;
        auto sponzaModel = importer.Import("Content/Models/Sponza/Sponza.gltf", true);

		//Entities
		auto sponzaEntity = ecsManager->CreateEntity();
		sponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		sponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(0,0,0));
		
		auto secondSponzaEntity = ecsManager->CreateEntity();
		secondSponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		secondSponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(50,0,0));
		
		auto thirdSponzaEntity = ecsManager->CreateEntity();
		thirdSponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		thirdSponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(50,50,0));

		auto dirLightEntity = ecsManager->CreateEntity();
		auto& lightTransform = dirLightEntity.Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
		lightTransform.SetEuler(90, 0, 0);
		dirLightEntity.Add<Waldem::Light>(Waldem::Vector3(1, 1, 1), 2.0f, Waldem::LightType::Directional, 100.0f);

		//do it after all entities set up
		ecsManager->Refresh();

		DrawSystems.Add((Waldem::ISystem*)new Waldem::ShadowmapRenderingSystem(ecsManager));
		DrawSystems.Add((Waldem::ISystem*)new Waldem::DeferredRenderingSystem(ecsManager));
		
		for (Waldem::ISystem* system : UpdateSystems)
		{
			system->Initialize(sceneData, inputManager);
		}
		
		for (Waldem::ISystem* system : DrawSystems)
		{
			system->Initialize(sceneData, inputManager);
		}
	}

	void DefaultScene::Update(float deltaTime)
	{
		for (Waldem::ISystem* system : UpdateSystems)
		{
			system->Update(deltaTime);
		}
	}

	void DefaultScene::Draw(float deltaTime)
	{
		for (Waldem::ISystem* system : DrawSystems)
		{
			system->Update(deltaTime);
		}
	}

	void DefaultScene::DrawUI(float deltaTime)
	{
	}
}
