#pragma once
#include "Mesh.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API Quad : public Mesh
    {
    public:
        Quad() : Mesh(QuadVertices, sizeof(QuadVertices), QuadIndices, sizeof(QuadIndices) / sizeof(uint32_t), nullptr, CalculateBoundingBox())
        {
            VB = Renderer::CreateVertexBuffer(QuadVertices, sizeof(QuadVertices));
            IB = Renderer::CreateIndexBuffer(QuadIndices, 6);
            IB->SetIndices(QuadIndices);
        }

    private:
        Vertex QuadVertices[4] =
        {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, 0},
            {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, 0},
            {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, 0},
            {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, 0}
        };
        uint32_t QuadIndices[6] = { 0, 2, 1, 1, 2, 3 };

        static BoundingBox CalculateBoundingBox()
        {
            BoundingBox bBox;
            bBox.Min = Vector3(-0.5f, -0.5f, 0.0f);
            bBox.Max = Vector3(0.5f, 0.5f, 0.0f);
            return bBox;
        }
    };
}
