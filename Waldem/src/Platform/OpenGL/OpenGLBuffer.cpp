#include "wdpch.h"
#include "OpenGLBuffer.h"

#include "glad/glad.h"

///////////////////////////////////////////////////////////////////////////////////////////
//  VertexBuffer  /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

Waldem::OpenGLVertexBuffer::OpenGLVertexBuffer(void* vertices, uint32_t size)
{
    glCreateBuffers(1, &RendererID);
	OpenGLVertexBuffer::Bind();
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

Waldem::OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
	glDeleteBuffers(1, &RendererID);
}

void Waldem::OpenGLVertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, RendererID);
}

void Waldem::OpenGLVertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////
//  IndexBuffer  //////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

Waldem::OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : Count(count)
{
	glCreateBuffers(1, &RendererID);
	OpenGLIndexBuffer::Bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

Waldem::OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
	glDeleteBuffers(1, &RendererID);
}

void Waldem::OpenGLIndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RendererID);
}

void Waldem::OpenGLIndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
