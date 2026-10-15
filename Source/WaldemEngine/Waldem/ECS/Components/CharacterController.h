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
        FIELD()
        float CoyoteTime = 0.12f;

        Vector3 MoveVelocity = Vector3(0.0f);
        bool JumpRequested = false;
        bool IsGrounded = false;
        float CoyoteTimer = 0.0f;
        
        CharacterController() {}
    };
}
#include "CharacterController.generated.h"
