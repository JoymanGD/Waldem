#pragma once
#include "Transform.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Renderer/PixelShader.h"
#include "Waldem/Renderer/VertexArray.h"
#include "glm/gtc/quaternion.hpp"

namespace Waldem
{
    
    class WALDEM_API Mesh
    {
    public:
        Mesh(float* vertices, uint32_t verticesSize, uint32_t* indices, uint32_t indicesAmount, PixelShader* shader, const BufferLayout& layout);

        void Bind();
        void Unbind();
        void SetShaderParam(ShaderParamType type, const GLchar* name, void* value);
        
        std::shared_ptr<IndexBuffer> IB;
        std::shared_ptr<VertexBuffer> VB;
        std::shared_ptr<VertexArray> VA;
        std::shared_ptr<PixelShader> MeshShader;
        Transform WorldTransform;
        std::vector<ShaderParam*> ShaderParameters;
    };
}
