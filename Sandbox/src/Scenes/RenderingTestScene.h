#pragma once

#include "Waldem/Audio/Audio.h"
#include "Waldem/ECS/Components/AudioSource.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Renderer/Resources/ResourceManager.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/ECS/Entity.h"
#include "Waldem/ECS/ECSManager.h"

namespace Sandbox
{
    class RenderingTestScene : public Waldem::GameScene
    {
    public:
        void Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, Waldem::ECSManager* ecsManager, Waldem::ResourceManager* resourceManager) override
        {        	
			Waldem::ModelImporter importer;

			//Entities
			auto sponzaModel = importer.Import("Content/Models/Sponza/Sponza2.gltf");
			//firstSponza
			int index = 0;
			for (Waldem::CMesh* mesh : sponzaModel->GetMeshes())
			{
				auto entity = ecsManager->CreateEntity("Sponza1_" + std::to_string(index++));
				entity->Add<Waldem::MeshComponent>(mesh);
				entity->Add<Waldem::Transform>(mesh->ObjectMatrix);
				Entities.Add(std::move(entity));
			}
			
			//secondSponza
			index = 0;
			for (Waldem::CMesh* mesh : sponzaModel->GetMeshes())
			{
				auto entity = ecsManager->CreateEntity("Sponza2_" + std::to_string(index++));
				entity->Add<Waldem::MeshComponent>(mesh);
				Waldem::Transform transform = mesh->ObjectMatrix;
				transform.Translate({50, 0, 0});
				entity->Add<Waldem::Transform>(transform);
				Entities.Add(entity);
			}

			//water plane
			auto waterTexture = resourceManager->LoadTexture("Content/Textures/WaterColor.png");
			auto waterPlaneModel = importer.Import("Content/Models/WaterPlane.glb");
			for (Waldem::CMesh* mesh : waterPlaneModel->GetMeshes())
			{
				mesh->SetMaterial(new Waldem::Material(waterTexture, nullptr, nullptr));
				auto waterPlaneEntity = ecsManager->CreateEntity("WaterPlane");
				waterPlaneEntity->Add<Waldem::MeshComponent>(mesh);
				Waldem::Transform transform = mesh->ObjectMatrix;
				transform.Translate({0,20,0});
				waterPlaneEntity->Add<Waldem::Transform>(transform);
				waterPlaneEntity->Add<Waldem::Ocean>();
				Entities.Add(waterPlaneEntity);
			}

        	//dir light
			auto dirLightEntity = ecsManager->CreateEntity("DirLight");
			auto& lightTransform = dirLightEntity->Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
			lightTransform.SetEuler(90, 0, 0);
        	dirLightEntity->Add<Waldem::Light>(Waldem::Vector3(1, 1, 1), 55.0f);
			Entities.Add(dirLightEntity);

        	//point light 1
        	auto pointLight1Entity = ecsManager->CreateEntity("PointLight1");
        	pointLight1Entity->Add<Waldem::Transform>(Waldem::Vector3(2, 1, 2));
        	pointLight1Entity->Add<Waldem::Light>(Waldem::Vector3(1, .3f, 1), 10.0f, 2.0f);
			Entities.Add(pointLight1Entity);
        	// pointLight1Entity->Add<Waldem::AudioSource>(Waldem::Audio::Load("Content/Sounds/Nuvaon"), 20.0f, true, 1.0f, true);

        	//point light 2
        	auto pointLight2Entity = ecsManager->CreateEntity("PointLight2");
        	pointLight2Entity->Add<Waldem::Transform>(Waldem::Vector3(-2, 1, -2));
        	pointLight2Entity->Add<Waldem::Light>(Waldem::Vector3(1, 1, .3f), 10.0f, 2.0f);
			Entities.Add(pointLight2Entity);

        	//spot light 1
        	auto spotLight1Entity = ecsManager->CreateEntity("SpotLight1");
        	spotLight1Entity->Add<Waldem::Transform>(Waldem::Vector3(-2, 1, -2));
        	spotLight1Entity->Add<Waldem::Light>(Waldem::Vector3(1, 1, .3f), 10.0f, 5.0f, 21, 0.001);
			Entities.Add(spotLight1Entity);

        	// auto postProcessEntity = ecsManager->CreateEntity("PostProcess");
        	// postProcessEntity->Add<Waldem::BloomPostProcess>();

			//do it after all entities set up
			ecsManager->Refresh();
        }
    };
}