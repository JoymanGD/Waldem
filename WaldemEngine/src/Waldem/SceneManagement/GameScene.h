#pragma once

#include "Waldem/ECS/ECSManager.h"
#include "Waldem/ECS/Entity.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/Serialization/Serializable.h"

namespace Waldem
{
    class WALDEM_API GameScene : IScene, ISerializable
    {
    public:
        virtual ~GameScene();
        void Initialize(SceneData* sceneData, InputManager* inputManager, ECSManager* ecsManager, ResourceManager* resourceManager) override;
        void Draw(float deltaTime) override;
        void Update(float deltaTime) override;
        void FixedUpdate(float fixedDeltaTime) override;
        void DrawUI(float deltaTime) override;
        
        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

    protected:
        WArray<ISystem*> UpdateSystems;
        WArray<ISystem*> DrawSystems;
        WArray<ISystem*> PhysicsSystems;
        WArray<ISystem*> Widgets;
        WArray<CEntity*> Entities;
        ECSManager* EcsManager = nullptr;
    };
}