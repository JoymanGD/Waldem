#pragma once

#include "Waldem/ECS/ECS.h"

namespace Waldem::ModelSpawner
{
    ECS::Entity WALDEM_API InstantiateModel(const Path& modelPath, ECS::Entity parent = {});
}
