#pragma once

namespace Waldem
{
    struct WALDEM_API SceneEntity
    {
        uint64 UUID = 0;
        uint64 ParentId = 0;
        float HierarchyDepth = 0;
        float HierarchySlot = -1;
        bool VisibleInHierarchy = true;
    };
}
