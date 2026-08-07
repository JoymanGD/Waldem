#pragma once
#include "ComponentBase.h"
#include "Waldem/Renderer/Model/TerrainMesh.h"

namespace Waldem
{
#define DEFAULT_TERRAIN_SIZE 100.0f
    
    struct PhysXTerrainSample
    {
        int16 Height;
        uint8 Material1;
        uint8 Material2;
    };

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

        ResizableBuffer* VertexBuffer;
        ResizableBuffer* IndexBuffer;
        ResizableBuffer* PhysXSamplesBuffer;
        Buffer* DataBuffer;
        DrawIndexedCommand DrawCommand;
        int VertexCount;
        uint IndexCount;

        Terrain()
        {
        }
    };
}
#include "Terrain.generated.h"
