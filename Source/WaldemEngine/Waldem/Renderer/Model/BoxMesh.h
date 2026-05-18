#pragma once
#include "StaticMesh.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API BoxMesh : public StaticMesh
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