#pragma once

#include "Waldem/ECS/Systems/CoreSystem.h"
#include "Waldem/ECS/ECS.h"

namespace Waldem
{
    class WALDEM_API PhysXSystem : public ICoreSystem
    {
    public:
        static Vector3 GetGravity();
        static void SetGravity(const Vector3& gravity);
        static bool LookAt(ECS::Entity entity, const Vector3& target);
        static bool RotateTowards(ECS::Entity entity, const Vector3& target, float maxDegreesDelta);
        void Initialize() override;
        void Deinitialize() override;
    };
}
