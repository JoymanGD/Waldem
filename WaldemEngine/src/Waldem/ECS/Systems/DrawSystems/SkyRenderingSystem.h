#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/ECS/Components\Sky.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"

namespace Waldem
{
    
    class WALDEM_API SkyRenderingSystem : public ISystem
    {
        
    public:
        SkyRenderingSystem() {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
        {

        }
    };
}
