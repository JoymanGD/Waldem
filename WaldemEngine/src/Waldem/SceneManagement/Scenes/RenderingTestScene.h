#pragma once

#include "Waldem/Audio/Audio.h"
#include "Waldem/ContentManagement/ModelImporter.h"
#include "Waldem/ECS/Components/AudioSource.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Resources/ResourceManager.h"
#include "Waldem/ContentManagement/ModelImporter.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/Model/Mesh.h"

namespace Waldem
{
    class RenderingTestScene : public GameScene
    {
    public:
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override
        {
			CModelImporter importer;

			//Entities
			auto sponzaModel = importer.Import("Sponza/Sponza2.gltf");
			//firstSponza
			int index = 0;
			for (CMesh* mesh : sponzaModel->GetMeshes())
			{
				auto sponza1Name = "Sponza1_" + std::to_string(index++);
				auto entity = ECS::CreateEntity(sponza1Name.c_str());
				entity.set<MeshComponent>(MeshComponent(mesh));
				auto transform = Transform(mesh->ObjectMatrix);
				entity.set<Transform>(transform);
			}
			
			//secondSponza
			index = 0;
			for (CMesh* mesh : sponzaModel->GetMeshes())
			{
				auto sponza2Name = "Sponza2_" + std::to_string(index++);
				auto entity = ECS::CreateEntity(sponza2Name.c_str());
				entity.set<MeshComponent>(MeshComponent(mesh));
				Transform transform = mesh->ObjectMatrix;
				transform.Translate({50, 0, 0});
				entity.set<Transform>(transform);
			}

			//water plane
			// auto waterTexture = resourceManager->LoadTexture("Content/Textures/WaterColor.png");
			// auto waterPlaneModel = importer.Import("Content/Models/WaterPlane.glb");
			// for (CMesh* mesh : waterPlaneModel->GetMeshes())
			// {
			// 	mesh->SetMaterial(new Material(waterTexture, nullptr, nullptr));
			// 	auto waterPlaneEntity = ECS::CreateEntity("WaterPlane");
			// 	waterPlaneEntity.add<MeshComponent>(mesh);
			// 	Transform transform = mesh->ObjectMatrix;
			// 	transform.Translate({0,20,0});
			// 	waterPlaneEntity.add<Transform>(transform);
			// 	waterPlaneEntity.add<Ocean>();
			// 	Entities.Add(waterPlaneEntity);
			// }

        	//dir light
			auto dirLightEntity = ECS::CreateEntity("DirLight");
        	auto lightTransform = Transform();
        	lightTransform.SetRotation(90, 0, 0);
			dirLightEntity.set<Transform>(lightTransform);
        	dirLightEntity.set<Light>(Light(Vector3(1, 1, 1), 55.0f));

        	//point light 1
        	auto pointLight1Entity = ECS::CreateEntity("PointLight1");
        	pointLight1Entity.set<Transform>(Transform(Vector3(2, 1, 2)));
        	pointLight1Entity.set<Light>(Light(Vector3(1, .3f, 1), 10.0f, 2.0f));

        	//point light 2
        	auto pointLight2Entity = ECS::CreateEntity("PointLight2");
        	pointLight2Entity.set<Transform>(Transform(Vector3(-2, 1, -2)));
        	pointLight2Entity.set<Light>(Light(Vector3(1, 1, .3f), 10.0f, 2.0f));

        	//spot light 1
        	auto spotLight1Entity = ECS::CreateEntity("SpotLight1");
        	spotLight1Entity.set<Transform>(Transform(Vector3(-2, 1, -2)));
        	spotLight1Entity.set<Light>(Light(Vector3(1, 1, .3f), 10.0f, 5.0f, 21, 0.001));

        	// auto postProcessEntity = ECS::CreateEntity("PostProcess");
        	// postProcessEntity.add<BloomPostProcess>();
        }
    };
}