#pragma once

namespace Waldem
{
    struct WALDEM_API PlayerController
    {
        COMPONENT(PlayerController)
            FIELD(float, MovementSpeed)
            FIELD(bool, RotateTowardMovementDirection)
            FIELD(float, RotationSpeed)
        END_COMPONENT()
        
        float MovementSpeed = 1.0f;
        bool RotateTowardMovementDirection = false;
        float RotationSpeed = .2f;
    };
}
