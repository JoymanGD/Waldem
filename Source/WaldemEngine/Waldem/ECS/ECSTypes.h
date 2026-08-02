#pragma once

#include "../../../../Vendor/flecs/include/flecs.h"

namespace Waldem
{
    namespace ECS
    {
        struct OnFixedUpdate {};
        struct OnLateUpdate {};
        struct OnDraw {};
        struct OnGUI {};
        
        using Entity = flecs::entity;
        using EntityT = flecs::entity_t;
        using Id = flecs::id;
        using IdT = flecs::id_t;
        using Iter = flecs::iter;
        using TypeSerializer = flecs::TypeSerializer;
        using MetaOp = flecs::meta::op_t;
        using ComponentRegisterFn = void(*)(flecs::world&);
    }
}
