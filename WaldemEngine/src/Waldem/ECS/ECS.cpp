#include "wdpch.h"

#include "ECS.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"

namespace Waldem
{
    namespace ECS
    {
        void RegisterComponents()
        {
            Transform::RegisterComponent(World);
            RigidBody::RegisterComponent(World);
        }
    }
}
