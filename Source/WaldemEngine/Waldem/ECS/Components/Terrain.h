#pragma once
#include "ComponentBase.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API Terrain
    {
        FIELD()
        int Resolution = 128;
        FIELD()
        float Height = 1.f;

        uint InitializedResolution = 128;

        Terrain() {}
    };
}
#include "Terrain.generated.h"
