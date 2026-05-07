#pragma once
#include "Waldem/ECS/ECS.h"

#define BIND(runtime, name) (runtime)->RegisterInternalCall("Waldem.InternalCalls::" #name, &name)

namespace Waldem
{
    inline ECS::Entity GetEntityChecked(uint64_t entityId)
    {
        if(entityId == 0)
            return {};
        ECS::Entity entity = ECS::World.entity(static_cast<ECS::EntityT>(entityId));
        if(!entity.is_alive())
            return {};
        return entity;
    }
}
