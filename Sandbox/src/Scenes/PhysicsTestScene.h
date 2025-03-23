#pragma once

#include "ecs.h"
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Renderer/Resources/ResourceManager.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/PhysicsComponent.h"
#include "Waldem/Renderer/Light.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Types/MathTypes.h"
#include "Waldem/Types/WArray.h"

namespace Sandbox
{
    class PhysicsTestScene : public Waldem::Scene
    {
    public:
        void Initialize(Waldem::SceneData* sceneData, Waldem::InputManager* inputManager, ecs::Manager* ecsManager, Waldem::ResourceManager* resourceManager) override
        {
			Waldem::ModelImporter importer;

			//Entities
			auto sceneModel = importer.Import("Content/Models/PhysicsTestScene.glb");
        	
			int index = 0;
			for (Waldem::CMesh* mesh : sceneModel->GetMeshes())
			{
				auto entity = ecsManager->CreateEntity("PhysicsTestScene_" + std::to_string(index++) + "_" + mesh->Name);
				entity.Add<Waldem::MeshComponent>(mesh);
				entity.Add<Waldem::Transform>(mesh->ObjectMatrix);
				entity.Add<Waldem::PhysicsComponent>();
				entity.Add<Waldem::ColliderComponent>(Waldem::WD_COLLIDER_TYPE_MESH, Waldem::MeshColliderData{ mesh });
			}
			
			auto dirLightEntity = ecsManager->CreateEntity("DirectionalLight");
			auto& lightTransform = dirLightEntity.Add<Waldem::Transform>(Waldem::Vector3(0, 0, 0));
			lightTransform.SetEuler(90, 0, 0);
			dirLightEntity.Add<Waldem::Light>(Waldem::Vector3(1, 1, 1), 20.0f);

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
