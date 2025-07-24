#pragma once
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem
{
    struct WALDEM_API ModelComponent
    {
        CModel* Model;
        
        ModelComponent() = default;
        
        ModelComponent(CModel* model) : Model(model) {}
    };
}
