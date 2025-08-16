#pragma once
#include "Waldem/Types/String.h"

namespace Waldem
{
    struct WALDEM_API SceneEntity
    {
        uint64 ParentId;
        float HierarchySlot;
        bool VisibleInHierarchy = true;
    };
}
