#pragma once

#include "Waldem/ECS/Components/ComponentBase.h"

namespace Waldem
{
    COMPONENT(EditorOnly)
    struct Sky
    {
        FIELD()
        Vector3 SkyZenithColor = Vector3(0.20f, 0.45f, 0.90f);
        FIELD()
        Vector3 SkyHorizonColor = Vector3(0.60f, 0.70f, 0.90f);
        FIELD()
        Vector3 GroundColor = Vector3(0.30f, 0.32f, 0.35f);
        FIELD()
        Vector3 SunDirection = Vector3(0.0f, 1.0f, 4.0f);
        
        Sky() {}
    };
}
#include "Sky.generated.h"