#include <wdpch.h>
#include "DefaultScene.h"
#include "Waldem/Application.h"
#include "Waldem/ECS/Components/MainCamera.h"
#include "Waldem/ECS/Components/ModelComponent.h"
#include "Waldem/ECS/Systems/DebugSystem.h"
#include "Waldem/ECS/Systems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/ForwardRenderingSystem.h"
#include "Waldem/ECS/Systems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Systems/PostProcessSystem.h"
#include "Waldem/ECS/Systems/ShadowmapRenderingSystem.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/World/Camera.h"
namespace Sandbox
{
	void DefaultScene::Initialize(Waldem::SceneData* sceneData)
	{
		Waldem::ModelImporter importer;

		//Entities
		auto sponzaModel = importer.Import("Content/Models/Sponza/Sponza.gltf", true);
		
		auto sponzaEntity = ECSManager.CreateEntity();
		sponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		sponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(0,0,0));
		
		auto secondSponzaEntity = ECSManager.CreateEntity();
		secondSponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		secondSponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(50,0,0));
		
		auto thirdSponzaEntity = ECSManager.CreateEntity();
		thirdSponzaEntity.Add<Waldem::ModelComponent>(sponzaModel);
		thirdSponzaEntity.Add<Waldem::Transform>(Waldem::Vector3(50,50,0));

		auto cameraEntity = ECSManager.CreateEntity();
		float aspectRatio = sceneData->Window->GetWidth() / sceneData->Window->GetHeight();
		cameraEntity.Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
		cameraEntity.Add<Waldem::Camera>(70.0f, aspectRatio, 0.001f, 1000.0f, 100.0f, 30.0f);
		cameraEntity.Add<Waldem::MainCamera>();

		auto dirLightEntity = ECSManager.CreateEntity();
		auto& lightTransform = dirLightEntity.Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
		lightTransform.SetEuler(90, 0, 0);
		dirLightEntity.Add<Waldem::Light>(Waldem::Vector3(1, 1, 1), 2.0f, Waldem::LightType::Directional, 100.0f);

		//do it after all entities set up
		ECSManager.Refresh();

		//Systems
		UpdateSystems.Add((Waldem::ISystem*)new Waldem::FreeLookCameraSystem(&ECSManager));
		UpdateSystems.Add((Waldem::ISystem*)new Waldem::DebugSystem(&ECSManager));

		DrawSystems.Add((Waldem::ISystem*)new Waldem::ShadowmapRenderingSystem(&ECSManager));
		DrawSystems.Add((Waldem::ISystem*)new Waldem::DeferredRenderingSystem(&ECSManager));
		// DrawSystems.Add((Waldem::ISystem*)new Waldem::ForwardRenderingSystem(&ECSManager));
		// DrawSystems.Add((Waldem::ISystem*)new Waldem::PostProcessSystem(&ECSManager));
		
		for (Waldem::ISystem* system : UpdateSystems)
		{
			system->Initialize(sceneData);
		}
		
		for (Waldem::ISystem* system : DrawSystems)
		{
			system->Initialize(sceneData); 
		}
	}

	void DefaultScene::Update(Waldem::SceneData* sceneData, float deltaTime)
	{
		for (Waldem::ISystem* system : UpdateSystems)
		{
			system->Update(sceneData, deltaTime);
		}
	}

	void DefaultScene::Draw(Waldem::SceneData* sceneData, float deltaTime)
	{
		for (Waldem::ISystem* system : DrawSystems)
		{
			system->Update(sceneData, deltaTime);
		}
	}
}