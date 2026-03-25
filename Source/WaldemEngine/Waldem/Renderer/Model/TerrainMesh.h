#pragma once
#include "PlaneMesh.h"

namespace Waldem
{
    class WALDEM_API TerrainMesh : public PlaneMesh
    {
    public:
        TerrainMesh(int resolution, Path materialPath = "") : PlaneMesh("Terrain", Point2(resolution, resolution), materialPath)
        {
        }
    };
}