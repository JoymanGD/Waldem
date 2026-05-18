#pragma once
#include "ComponentBase.h"
#include "Waldem/Renderer/Model/StaticMesh.h"
#include "Waldem/Renderer/Model/TerrainMesh.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API Terrain
    {
        FIELD()
        int Resolution = 128;

        uint InitializedResolution = 128;

        Terrain() {}
    };
}
#include "Terrain.generated.h"
