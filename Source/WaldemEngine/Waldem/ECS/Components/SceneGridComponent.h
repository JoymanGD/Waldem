#pragma once

#include "ComponentBase.h"

namespace Waldem
{
    enum SceneGridCellType : int
    {
        SceneGridCell_Empty = 0,
        SceneGridCell_Mesh = 1,
        SceneGridCell_Light = 2,
        SceneGridCell_Other = 3,
    };

    struct SceneGridCellData
    {
        Point3 Position = Point3(0);
        int Depth = 0;
        int Type = SceneGridCell_Empty;
    };

    struct SceneGridData
    {
        Vector3 CenterPosition = Vector3(0.0f);
        Point3 Size = Point3(0);
        int CellSize = 1;
        int SubdivisionFactor = 2;
    };

    COMPONENT()
    struct WALDEM_API SceneGridComponent
    {
        FIELD()
        Vector3 GridDimensions = Vector3(10.0f, 4.0f, 10.0f);
        FIELD()
        float CellSize = 1.0f;
        FIELD()
        Vector3 LocalOffset = Vector3(0.0f);
        FIELD()
        bool CenterOnTransform = true;
        FIELD()
        bool Visualize = true;
        FIELD()
        bool VisualizeOnlyOccupiedCells = false;
        FIELD()
        bool SubdivideOccupiedCells = false;
        FIELD()
        float SubdivisionLevels = 1.0f;
        FIELD()
        float SubdivisionFactor = 2.0f;

        SceneGridData GridData = {};
        WArray<SceneGridCellData> Cells;

        SceneGridComponent() = default;
    };
}
#include "SceneGridComponent.generated.h"
