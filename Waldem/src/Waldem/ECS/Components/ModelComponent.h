#pragma once
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem
{
    struct ModelComponent
    {
        ModelComponent(Model* model) : Model(model) {}
        Model* Model;
    };
}
