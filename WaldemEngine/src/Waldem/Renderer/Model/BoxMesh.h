#pragma once
#include "Mesh.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API BoxMesh : public CMesh
    {
    private:
        uint32_t BufferSize = sizeof(Line) * 12;

    public:
        BoxMesh()
        {
            VertexBuffer = Renderer::CreateBuffer("BoxMeshVertexBuffer", BufferType::VertexBuffer, BufferSize, sizeof(LineVertex));
        }
    };
}