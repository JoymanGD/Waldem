#pragma once
#include <flecs.h>

namespace Waldem
{
    #define COMPONENT(TYPE) \
        static void RegisterComponent(flecs::world& world) { \
        auto component = world.component<TYPE>()
    
    #define FIELD(TYPE, NAME) \
        .member<TYPE>(#NAME)
    
    #define END_COMPONENT() ; }
}
