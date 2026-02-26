#pragma once

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API PlayerController
    {        
        FIELD()
        float MovementSpeed = 1.0f;
        FIELD()
        bool RotateTowardMovementDirection = false;
        FIELD()
        float RotationSpeed = .2f;
        
        PlayerController() {}
    };
}
#include "PlayerController.generated.h"
