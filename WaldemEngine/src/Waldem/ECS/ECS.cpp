#include "wdpch.h"

#include "ECS.h"

#include "Components/AudioSource.h"
#include "Components/Light.h"
#include "Components/MeshComponent.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Waldem/Editor/AssetReference.h"

namespace Waldem
{
    namespace ECS
    {
        void RegisterTypes()
        {
            World.component<Vector3>("Vector3")
                .member<float>("x")
                .member<float>("y")
                .member<float>("z");
            
            World.component<WString>()
                .opaque(flecs::String) // Opaque type that maps to string
                    .serialize([](const flecs::serializer *s, const WString *data) {
                        const char *str = data->C_Str();
                        return s->value(flecs::String, &str); // Forward to serializer
                    })
                    .assign_string([](WString* data, const char *value) {
                        *data = value; // Assign new value to std::string
                    });

            World.component<AssetReference>()
                .member<WString>("Reference");
            
            // World.component<AssetReference>("AssetReference")
            //     .member<flecs::uptr_t>("Reference");
        }

        void RegisterComponents()
        {
            Transform::RegisterComponent(World);
            RigidBody::RegisterComponent(World);
            AudioSource::RegisterComponent(World);
            MeshComponent::RegisterComponent(World);
            Light::RegisterComponent(World);
        }
    }
}
