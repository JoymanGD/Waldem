#pragma once

#include "Waldem/ECS/Systems/System.h"
#include "Waldem/SceneManagement/Scene.h"

namespace Waldem
{
    class WALDEM_API GameScene : IScene
    {
    public:
        virtual ~GameScene();
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override;
        void Draw(float deltaTime) override;
        void Update(float deltaTime) override; 
        void FixedUpdate(float fixedDeltaTime) override;
        void DrawUI(float deltaTime) override;
        
        void Serialize(Path& outPath);
        void Deserialize(Path& inPath);

    protected:
        WArray<ISystem*> UpdateSystems;
        WArray<ISystem*> DrawSystems;
        WArray<ISystem*> PhysicsSystems;
        WArray<ISystem*> Widgets;
    };
}