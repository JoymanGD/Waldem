#include "wdpch.h"
#include "OpenGLBuffer.h"
#include "glad/glad.h"

namespace Waldem
{
	///////////////////////////////////////////////////////////////////////////////////////////
	//  VertexBuffer  /////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////

	OpenGLVertexBuffer::OpenGLVertexBuffer(void* vertices, uint32_t size)
	{
	    glCreateBuffers(1, &BufferID);
		OpenGLVertexBuffer::Bind();
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &BufferID);
	}

	void OpenGLVertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, BufferID);
	}

	void OpenGLVertexBuffer::Unbind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//  IndexBuffer  //////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : Count(count)
	{
		glCreateBuffers(1, &BufferID);
		OpenGLIndexBuffer::Bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		glDeleteBuffers(1, &BufferID);
	}

	void OpenGLIndexBuffer::Bind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferID);
	}

	void OpenGLIndexBuffer::Unbind() const
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//  StorageBuffer  ////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////

	OpenGLStorageBuffer::OpenGLStorageBuffer(void* data, size_t size)
	{
		glCreateBuffers(1, &BufferID);
		OpenGLStorageBuffer::Bind();
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_DRAW);
	}

	OpenGLStorageBuffer::~OpenGLStorageBuffer()
	{
		glDeleteBuffers(1, &BufferID);
	}

	void OpenGLStorageBuffer::Bind() const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, BufferID);
	}

	void OpenGLStorageBuffer::Unbind() const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}
}