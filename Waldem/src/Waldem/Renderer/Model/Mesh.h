#pragma once
#include "Material.h"
#include "Transform.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Renderer/VertexArray.h"
#include "glm/gtc/quaternion.hpp"
#include "Waldem/Renderer/Pipeline.h"

namespace Waldem
{
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec4 Color;
        glm::vec3 UV;
    };
    
    class WALDEM_API Mesh
    {
    public:
        Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indices, uint32_t indicesAmount, const BufferLayout& layout, Material material);

        void Initialize(Pipeline* pipeline);

        IndexBuffer* IB;
        VertexBuffer* VB;
        VertexArray* VA;
        Material MeshMaterial;
    };
}