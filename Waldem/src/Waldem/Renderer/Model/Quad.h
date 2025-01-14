#pragma once
#include "Mesh.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API Quad : public Mesh
    {
    private:
        Vertex QuadVertices[4] =
        {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, 0},
            {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, 0},
            {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, 0},
            {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, 0}
        };
        uint32_t QuadIndices[6] = { 0, 2, 1, 1, 2, 3 };

        static BoundingBox CalculateBoundingBox()
        {
            BoundingBox bBox;
            bBox.Min = Vector3(-0.5f, -0.5f, 0.0f);
            bBox.Max = Vector3(0.5f, 0.5f, 0.0f);
            return bBox;
        }

    public:
        Quad()
        {
            VertexBuffer = Renderer::CreateBuffer("QuadVertexBuffer", BufferType::VertexBuffer, QuadVertices, sizeof(QuadVertices));
            IndexBuffer = Renderer::CreateBuffer("QuadIndexBuffer", BufferType::IndexBuffer, QuadIndices, sizeof(QuadIndices));
            BBox = CalculateBoundingBox();

            CurrentMaterial = nullptr;

            ObjectMatrix = glm::identity<Matrix4>();
        }
    };
}
