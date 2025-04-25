#pragma once

#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Resources/ResourceManager.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Types/MathTypes.h"
#include "Waldem/Types/WArray.h"
#include "Waldem/ECS/Entity.h"
#include "Waldem/ECS/ECSManager.h"

namespace Waldem
{
    class PhysicsTestScene : public IScene
    {
    public:
        void Initialize(SceneData* sceneData, InputManager* inputManager, ECSManager* ecsManager, ResourceManager* resourceManager) override
        {
			ModelImporter importer;

			//Entities
			auto sceneModel = importer.Import("Content/Models/PhysicsTestScene.glb");
        	
			int index = 0;
			for (CMesh* mesh : sceneModel->GetMeshes())
			{
				auto entity = ecsManager->CreateEntity("PhysicsTestScene_" + std::to_string(index) + "_" + mesh->Name);
				entity->Add<MeshComponent>(mesh);
				entity->Add<Transform>(mesh->ObjectMatrix);
				auto collider = ColliderComponent(WD_COLLIDER_TYPE_MESH, MeshColliderData(mesh));
				entity->Add<ColliderComponent>(collider);
				entity->Add<RigidBody>(index < 5, true, 1.0f, &collider); //we set first 5 meshes as kinematic since its floor and walls

				index++;

				// if(index > 5) break;
			}
			
			auto dirLightEntity = ecsManager->CreateEntity("DirectionalLight");
			auto& lightTransform = dirLightEntity->Add<Transform>(Vector3(0, 0, 0));
			lightTransform.SetEuler(90, 0, 0);
			dirLightEntity->Add<Light>(Vector3(1, 1, 1), 20.0f);

			//do it after all entities set up
			ecsManager->Refresh();
			
			for (ISystem* system : UpdateSystems)
			{
				system->Initialize(sceneData, inputManager, resourceManager);
			}
			
			for (ISystem* system : DrawSystems)
			{
				system->Initialize(sceneData, inputManager, resourceManager);
			}
        }
    	
    	void Update(float deltaTime) override
        {
        	for (ISystem* system : UpdateSystems)
        	{
        		system->Update(deltaTime);
        	}
        }
    	
        void FixedUpdate(float fixedDeltaTime) override {}
        
    	void Draw(float deltaTime) override
        {
        	for (ISystem* system : DrawSystems)
        	{
        		system->Update(deltaTime);
        	}
        }
    	
        void DrawUI(float deltaTime) override {}

    private:
        WArray<ISystem*> UpdateSystems;
        WArray<ISystem*> DrawSystems;
    };
}
