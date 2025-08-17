#pragma once

#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    struct Sky
    {
        COMPONENT(Sky)
            FIELD(Vector3, SkyZenithColor)
            FIELD(Vector3, SkyHorizonColor)
            FIELD(Vector3, GroundColor)
            FIELD(Vector3, SunDirection)
            EDITOR_ONLY()
        END_COMPONENT()
        
        Vector3 SkyZenithColor = Vector3(0.20f, 0.45f, 0.90f);
        Vector3 SkyHorizonColor = Vector3(0.60f, 0.70f, 0.90f);
        Vector3 GroundColor = Vector3(0.30f, 0.32f, 0.35f);
        Vector3 SunDirection = Vector3(0.0f, 1.0f, 4.0f);
    };
}