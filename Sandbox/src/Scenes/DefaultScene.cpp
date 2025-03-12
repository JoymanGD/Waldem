#include <wdpch.h>
#include "DefaultScene.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/PhysicsComponent.h"
#include "Waldem/Renderer/Light.h"

namespace Sandbox
{
	void DefaultScene::Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, ecs::Manager* ecsManager, Waldem::ResourceManager* resourceManager)
	{
		Waldem::ModelImporter importer;

		//Entities
		auto sponzaModel = importer.Import("Content/Models/Sponza/Sponza2.gltf");
		//firstSponza
		int index = 0;
		for (Waldem::Mesh* mesh : sponzaModel->GetMeshes())
		{
			auto entity = ecsManager->CreateEntity("Sponza1_" + std::to_string(index++));
			entity.Add<Waldem::MeshComponent>(mesh);
			entity.Add<Waldem::Transform>(mesh->ObjectMatrix);
		}
		
		//secondSponza
		index = 0;
		for (Waldem::Mesh* mesh : sponzaModel->GetMeshes())
		{
			auto entity = ecsManager->CreateEntity("Sponza2_" + std::to_string(index++));
			entity.Add<Waldem::MeshComponent>(mesh);
			Waldem::Transform transform = mesh->ObjectMatrix;
			transform.Translate({50, 0, 0});
			entity.Add<Waldem::Transform>(transform);
			entity.Add<Waldem::PhysicsComponent>();
		}

		//water plane
		// auto waterTexture = resourceManager->LoadTexture("Content/Textures/WaterColor.png");
		// auto waterPlaneModel = importer.Import("Content/Models/WaterPlane.glb", true);
		// for (Waldem::Mesh* mesh : waterPlaneModel->GetMeshes())
		// {
		// 	mesh->SetMaterial(new Waldem::Material(waterTexture, nullptr, nullptr, nullptr));
		// 	auto waterPlaneEntity = ecsManager->CreateEntity("WaterPlane");
		// 	waterPlaneEntity.Add<Waldem::MeshComponent>(mesh);
		// 	Waldem::Transform transform = mesh->ObjectMatrix;
		// 	transform.Translate({0,20,0});
		// 	waterPlaneEntity.Add<Waldem::Transform>(transform);
		// 	waterPlaneEntity.Add<Waldem::Ocean>();
		// }
		
		auto dirLightEntity = ecsManager->CreateEntity("DirectionalLight");
		auto& lightTransform = dirLightEntity.Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
		lightTransform.SetEuler(90, 0, 0);
		dirLightEntity.Add<Waldem::Light>(Waldem::Vector3(1, 1, 1), 20.0f, Waldem::LightType::Directional, 100.0f);

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
