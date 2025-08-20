#pragma once
#include <flecs.h>
#include "Waldem/ECS/ECS.h"
#include <typeinfo>
#include "Waldem/ECS/Components/EditorComponent.h"
#include "Waldem/ECS/Components/Hidden.h"

namespace Waldem
{
    #define COMPONENT(TYPE) \
        TYPE() = default; \
        static void RegisterComponent(flecs::world& world) { \
        auto component = world.component<TYPE>() \
    
    #define FIELD(TYPE, NAME) \
        .member<TYPE>(#NAME)
            
    #define HIDDEN_FIELD(TYPE, NAME) \
        .member<TYPE>("___" #NAME)
        
    #define EDITOR_ONLY() \
        .add<EditorComponent>()
    
    #define END_COMPONENT() \
        ; \
        ECS::RegisteredComponents.Add(component.name().c_str(), component); \
        }
}
