#include <wdpch.h>
#include "DefaultScene.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/ECS/Systems/DebugSystem.h"
#include "Waldem/ECS/Systems/DeferredRenderingSystem.h"
#include "Waldem/Import/ModelImporter.h"
#include "imgui.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/EditorTransformsManipulationSystem.h"
#include "Waldem/ECS/Systems/ShadowmapRenderingSystem.h"

namespace Sandbox
{
	void DefaultScene::Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, ecs::Manager* ecsManager, Waldem::ResourceManager* resourceManager)
	{
		Waldem::ModelImporter importer;
        auto sponzaModel = importer.Import("Content/Models/Sponza/Sponza2.gltf", true);

		//Entities
		
		// //firstSponza
		// for (Waldem::Mesh* mesh : sponzaModel->GetMeshes())
		// {
		// 	auto entity = ecsManager->CreateEntity();
		// 	entity.Add<Waldem::MeshComponent>(mesh);
		// 	entity.Add<Waldem::Transform>(mesh->ObjectMatrix);
		// }
		//
		// //secondSponza
		// for (Waldem::Mesh* mesh : sponzaModel->GetMeshes())
		// {
		// 	auto entity = ecsManager->CreateEntity();
		// 	entity.Add<Waldem::MeshComponent>(mesh);
		// 	Waldem::Transform transform = mesh->ObjectMatrix;
		// 	transform.Translate({50, 0, 0});
		// 	entity.Add<Waldem::Transform>(transform);
		// }

		//water plane
		auto waterPlaneModel = importer.Import("Content/Models/WaterPlane.glb", true);
		for (Waldem::Mesh* mesh : waterPlaneModel->GetMeshes())
		{
			auto waterPlaneEntity = ecsManager->CreateEntity();
			waterPlaneEntity.Add<Waldem::MeshComponent>(mesh);
			Waldem::Transform transform = mesh->ObjectMatrix;
			transform.Translate({50, 0, 0});
			waterPlaneEntity.Add<Waldem::Transform>(transform);
		}
		
		// auto sponzaEntity = ecsManager->CreateEntity();
		// sponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		// sponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(0,0,0));
		// sponzaEntity.Add<Waldem::Selected>();
		//
		// auto secondSponzaEntity = ecsManager->CreateEntity();
		// secondSponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		// secondSponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(50,0,0));
		//
		// auto thirdSponzaEntity = ecsManager->CreateEntity();
		// thirdSponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		// thirdSponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(50,50,0));

		auto dirLightEntity = ecsManager->CreateEntity();
		auto& lightTransform = dirLightEntity.Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
		lightTransform.SetEuler(90, 0, 0);
		dirLightEntity.Add<Waldem::Light>(Waldem::Vector3(1, 1, 1), 2.0f, Waldem::LightType::Directional, 100.0f);

		//do it after all entities set up
		ecsManager->Refresh();
		
		for (Waldem::ISystem* system : UpdateSystems)
		{
			system->Initialize(sceneData, inputManager, resourceManager);
		}
		
		for (Waldem::ISystem* system : DrawSystems)
		{
			system->Initialize(sceneData, inputManager, resourceManager);
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
