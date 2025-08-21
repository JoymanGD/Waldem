#pragma once
#include "Waldem/Types/String.h"

namespace Waldem
{
    struct WALDEM_API SceneEntity
    {
        uint64 ParentId;
        float HierarchySlot = -1;
        bool VisibleInHierarchy = true;
    };
}
