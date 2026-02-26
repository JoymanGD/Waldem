#pragma once
#include "Waldem/ECS/ECS.h"
#include <typeinfo>
#include "Waldem/ECS/Components/EditorComponent.h"
#include "Waldem/ECS/Components/HiddenComponent.h"
#include "Waldem/ECS/Components/Hidden.h"

namespace Waldem
{
    // #define COMPONENT(TYPE) \
    //     TYPE() = default; \
    //     static void RegisterComponent(flecs::world& world) { \
    //     using ComponentType = TYPE; \
    //     auto component = world.component<TYPE>() \
    //
    // #define FIELD(NAME) \
    //     .member(#NAME, &ComponentType::NAME)
    //         
    // #define FIELD_TYPE(TYPE, NAME) \
    //     .member<TYPE>(#NAME)
    //         
    // #define HIDDEN_FIELD(TYPE, NAME) \
    //     .member<TYPE>("___" #NAME)
    //     
    // #define EDITOR_ONLY() \
    //     .add<EditorComponent>()
    //     
    // #define HIDDEN_COMPONENT() \
    //     .add<HiddenComponent>()
    //
    // #define END_COMPONENT() \
    //     ; \
    //     ECS::RegisteredComponents.Add(component.name().c_str(), component); \
    //     }

    #define COMPONENT(...)
    #define FIELD(...)
    // #define COMPONENT(x)
    // #define FIELD_TYPE(type, name)
    // #define END_COMPONENT()
}
