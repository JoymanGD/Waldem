#pragma once
#include "Mesh.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    struct QuadVertex
    {
        Vector3 Position;
        Vector2 UV;

        QuadVertex(float x, float y, float z, float u, float v) : Position(x, y, z), UV(u,v) {}
        QuadVertex(Vector3 position, Vector2 uv) : Position(position), UV(uv) {}
    };
    
    class WALDEM_API Quad : public CMesh
    {
    private:
        QuadVertex QuadVertices[4] =
        {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
            {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}
        };
        uint32_t QuadIndices[6] = { 0, 2, 1, 1, 2, 3 };

        static AABB CalculateBoundingBox()
        {
            AABB bBox;
            bBox.Min = Vector3(-0.5f, -0.5f, 0.0f);
            bBox.Max = Vector3(0.5f, 0.5f, 0.0f);
            return bBox;
        }

    public:
        Quad()
        {
            VertexBuffer = Renderer::CreateBuffer("QuadVertexBuffer", BufferType::VertexBuffer, sizeof(QuadVertices), sizeof(QuadVertex), QuadVertices);
            IndexBuffer = Renderer::CreateBuffer("QuadIndexBuffer", BufferType::IndexBuffer, sizeof(QuadIndices), sizeof(uint32_t), QuadIndices);
            BBox = CalculateBoundingBox();

            CurrentMaterial = nullptr;

            ObjectMatrix = glm::identity<Matrix4>();
        }
    };
}
