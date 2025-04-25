#pragma once
#include "Mesh.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API LineMesh : public CMesh
    {
    private:
        uint32_t BufferSize = sizeof(Line) * 2000;

    public:
        LineMesh()
        {
            VertexBuffer = Renderer::CreateBuffer("QuadVertexBuffer", BufferType::VertexBuffer, nullptr, BufferSize, sizeof(LineVertex));
        }
    };
}