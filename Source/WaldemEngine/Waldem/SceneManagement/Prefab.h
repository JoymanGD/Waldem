#pragma once

#include "Waldem/ECS/ECS.h"

namespace Waldem::Prefab
{
    bool WALDEM_API SaveEntityAsPrefab(ECS::Entity rootEntity, const Path& outPath);
    ECS::Entity WALDEM_API InstantiatePrefab(const Path& prefabPath, ECS::Entity parent = {});
}
