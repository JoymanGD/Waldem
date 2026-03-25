#pragma once
#include "Mesh.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Utils/GeometryUtils.h"

namespace Waldem
{
    class WALDEM_API PlaneMesh : public CMesh
    {
        float DEFAULT_UNIT_SIZE = 100.0f;
    public:
        PlaneMesh(WString name, Point2 resolution, Path materialPath = "")
        {
            Name = name;

            if(!materialPath.empty())
            {
                MaterialPath = materialPath;
            }
            
            ObjectMatrix = glm::identity<Matrix4>();
            BBox = AABB(Vector3(-0.5f, -0.01f, -0.5f), Vector3(0.5f, 0.01f, 0.5f));
            
            uint32 bufferSize = sizeof(Vertex) * resolution.x * resolution.y;
            
            for (uint32 y = 0; y < resolution.y; y++)
            {
                for (uint32 x = 0; x < resolution.x; x++)
                {
                    uint32 vertexIndex = y * resolution.x + x;
                    float u = (float)x / (resolution.x - 1);
                    float v = (float)y / (resolution.y - 1);
                    
                    Vertex vertex = {
                        Vector4((u - 0.5f) * DEFAULT_UNIT_SIZE, 0.0f, (v - 0.5f) * DEFAULT_UNIT_SIZE, 1.0f), // Position
                        Vector4(1, 1, 1, 1), // Color
                        Vector4(0.0f, 1.0f, 0.0f, 0.0f), // Normal
                        Vector4(1.0f, 0.0f, 0.0f, 0.0f), // Tangent
                        Vector4(0.0f, 0.0f, 1.0f, 0.0f), // Bitangent
                        Vector2(u, v) // UV
                    };
                    
                    VertexData.Add(vertex);

                    if(x < resolution.x - 1 && y < resolution.y - 1)
                    {
                        uint32 topLeft = vertexIndex;
                        uint32 topRight = vertexIndex + 1;
                        uint32 bottomLeft = vertexIndex + resolution.x;
                        uint32 bottomRight = vertexIndex + resolution.x + 1;
                        
                        IndexData.Add(topLeft);
                        IndexData.Add(bottomLeft);
                        IndexData.Add(topRight);
                        
                        IndexData.Add(topRight);
                        IndexData.Add(bottomLeft);
                        IndexData.Add(bottomRight);
                    }
                }
            }

            GeometryUtils::ExtractPositionsFromVertexData(VertexData, Positions);

            VertexBuffer = Renderer::CreateBuffer("PlaneVertexBuffer", BufferType::VertexBuffer, bufferSize, sizeof(Vertex), VertexData.GetData(), VertexData.GetSize());
            IndexBuffer = Renderer::CreateBuffer("PlaneIndexBuffer", BufferType::IndexBuffer, IndexData.GetSize(), sizeof(uint32), IndexData.GetData());
        }
    };
}
