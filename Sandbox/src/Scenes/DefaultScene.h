#pragma once

#include "ecs.h"
#include "Waldem/Audio/Audio.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Renderer/Resources/ResourceManager.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Model/Mesh.h"

namespace Sandbox
{
    class DefaultScene : public Waldem::Scene
    {
    public:
        void Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, ecs::Manager* ecsManager, Waldem::ResourceManager* resourceManager) override
        {        	
			Waldem::ModelImporter importer;

			//Entities
			auto sponzaModel = importer.Import("Content/Models/Sponza/Sponza2.gltf");
			//firstSponza
			int index = 0;
			for (Waldem::CMesh* mesh : sponzaModel->GetMeshes())
			{
				auto entity = ecsManager->CreateEntity("Sponza1_" + std::to_string(index++));
				entity.Add<Waldem::MeshComponent>(mesh);
				entity.Add<Waldem::Transform>(mesh->ObjectMatrix);
			}
			
			//secondSponza
			index = 0;
			for (Waldem::CMesh* mesh : sponzaModel->GetMeshes())
			{
				auto entity = ecsManager->CreateEntity("Sponza2_" + std::to_string(index++));
				entity.Add<Waldem::MeshComponent>(mesh);
				Waldem::Transform transform = mesh->ObjectMatrix;
				transform.Translate({50, 0, 0});
				entity.Add<Waldem::Transform>(transform);
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

        	//dir light
			auto dirLightEntity = ecsManager->CreateEntity("DirectionalLight");
			auto& lightTransform = dirLightEntity.Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
			lightTransform.SetEuler(90, 0, 0);
        	dirLightEntity.Add<Waldem::Light>(Waldem::Vector3(1, 1, 1), 20.0f);

        	//point light 1
        	auto pointLight1Entity = ecsManager->CreateEntity("PointLight1");
        	pointLight1Entity.Add<Waldem::Transform>(Waldem::Vector3(2, 1, 2));
        	pointLight1Entity.Add<Waldem::Light>(Waldem::Vector3(1, .3f, 1), 10.0f, 2.0f);

        	//point light 2
        	auto pointLight2Entity = ecsManager->CreateEntity("PointLight2");
        	pointLight2Entity.Add<Waldem::Transform>(Waldem::Vector3(-2, 1, -2));
        	pointLight2Entity.Add<Waldem::Light>(Waldem::Vector3(1, 1, .3f), 10.0f, 2.0f);

        	//spot light 1
        	auto spotLight1Entity = ecsManager->CreateEntity("SpotLight1");
        	spotLight1Entity.Add<Waldem::Transform>(Waldem::Vector3(-2, 1, -2));
        	spotLight1Entity.Add<Waldem::Light>(Waldem::Vector3(1, 1, .3f), 10.0f, 5.0f, 21, 0.001);

        	// auto postProcessEntity = ecsManager->CreateEntity("PostProcess");
        	// postProcessEntity.Add<Waldem::BloomPostProcess>();

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
    	
    	void Update(float deltaTime) override
        {
        	for (Waldem::ISystem* system : UpdateSystems)
        	{
        		system->Update(deltaTime);
        	}
        }
        
    	void Draw(float deltaTime) override
        {
        	for (Waldem::ISystem* system : DrawSystems)
        	{
        		system->Update(deltaTime);
        	}
        }
    	
        void DrawUI(float deltaTime) override {}

    private:
        Waldem::WArray<Waldem::ISystem*> UpdateSystems;
        Waldem::WArray<Waldem::ISystem*> DrawSystems;
    };
}
