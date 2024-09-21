#include "wdpch.h"
#include "OpenGLVertexArray.h"

#include "glad/glad.h"

namespace Waldem
{
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::None:
            return GL_NONE;
        case ShaderDataType::Float:
        case ShaderDataType::Float2:
        case ShaderDataType::Float3:
        case ShaderDataType::Float4:
        case ShaderDataType::Mat3:
        case ShaderDataType::Mat4:
            return GL_FLOAT;
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4:
            return GL_INT;
        case ShaderDataType::Bool:
            return GL_BOOL;
        }

        WD_CORE_ASSERT(false, "Uknown shader data type")

        return GL_NONE;
    }
    
    OpenGLVertexArray::OpenGLVertexArray()
    {
        glCreateVertexArrays(1, &RendererID);
    }

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        glDeleteVertexArrays(1, &RendererID);
    }

    void OpenGLVertexArray::Bind() const
    {
        glBindVertexArray(RendererID);
    }

    void OpenGLVertexArray::Unbind() const
    {
        glBindVertexArray(0);
    }

    void OpenGLVertexArray::AddVertexBuffer(VertexBuffer* vertexBuffer)
    {
        WD_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex buffer has no layout")

        Bind();
        vertexBuffer->Bind();

        uint32_t i = 0;

        const auto& layout = vertexBuffer->GetLayout();
		
        for (const auto& element : layout)
        {
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, element.GetComponentCount(), ShaderDataTypeToOpenGLBaseType(element.Type), element.Normalized, layout.GetStride(), (const void*)element.Offset);
			
            i++;
        }
        
        VertexBuffers.push_back(vertexBuffer);

        Unbind();
    }

    void OpenGLVertexArray::SetIndexBuffer(Waldem::IndexBuffer* indexBuffer)
    {
        Bind();
        indexBuffer->Bind();

        IndexBuffer = indexBuffer;

        Unbind();
    }
}
