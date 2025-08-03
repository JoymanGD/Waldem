#include "wdpch.h"

#include "ECS.h"

#include "Components/AudioSource.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"

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
        }

        void RegisterComponents()
        {
            Transform::RegisterComponent(World);
            RigidBody::RegisterComponent(World);
            AudioSource::RegisterComponent(World);
        }
    }
}
