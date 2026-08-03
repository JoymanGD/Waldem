#pragma once
#include "ComponentBase.h"
#include "Waldem/Renderer/Model/TerrainMesh.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API Terrain
    {
        FIELD()
        int Resolution = 512;
        FIELD()
        float Height = 100.f;
        FIELD()
        TextureReference Heightmap;
        FIELD()
        TextureReference Albedo;

        uint InitializedResolution = 512;
        Path InitializedReference = "Empty";
        float InitializedHeight = 1.f;

        Terrain()
        {
        }
    };
}
#include "Terrain.generated.h"
