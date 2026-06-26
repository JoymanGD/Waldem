#pragma once

#include "Waldem/ECS/Components/ComponentBase.h"
#include "Waldem/Types/MathTypes.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API CharacterController
    {
        FIELD()
        float CapsuleHeight = 3.0f;
        FIELD()
        float CapsuleRadius = 1.0f;
        FIELD()
        float JumpSpeed = 12.0f;
        FIELD()
        float GravityScale = 1.0f;

        Vector3 MoveVelocity = Vector3(0.0f);
        bool JumpRequested = false;
        bool IsGrounded = false;
        
        CharacterController() {}
    };
}
#include "CharacterController.generated.h"
