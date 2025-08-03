#pragma once
#include <flecs.h>
#include "Waldem/ECS/ECS.h"
#include <typeinfo>

namespace Waldem
{
    #define COMPONENT(TYPE) \
        static void RegisterComponent(flecs::world& world) { \
        auto component = world.component<TYPE>() \
        
    
    #define FIELD(TYPE, NAME) \
        .member<TYPE>(#NAME)
    
    #define END_COMPONENT() \
        ; \
        ECS::RegisteredComponents.Add(component.name().c_str(), component); \
        }
}
