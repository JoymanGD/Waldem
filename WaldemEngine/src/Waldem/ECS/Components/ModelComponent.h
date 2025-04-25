#pragma once
#include "Waldem/Renderer/Model/Model.h"
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API ModelComponent : IComponent<ModelComponent>
    {
        CModel* Model;
        
        ModelComponent() = default;
        
        ModelComponent(CModel* model) : Model(model) {}
        
        void Serialize(WDataBuffer& outData) override {}
        void Deserialize(WDataBuffer& inData) override {}
    };
}
