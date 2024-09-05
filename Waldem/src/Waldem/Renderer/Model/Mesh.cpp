#include "wdpch.h"
#include "Mesh.h"

namespace Waldem
{
	Mesh::Mesh(float* vertices, uint32_t verticesSize, uint32_t* indices, uint32_t indicesAmount, PixelShader* shader, const BufferLayout& layout)
	{
		MeshShader.reset(shader);
		
		VB.reset(VertexBuffer::Create(vertices, verticesSize));
		VB->SetLayout(layout);
		IB.reset(IndexBuffer::Create(indices, indicesAmount));
	    VA.reset(VertexArray::Create());

		VA->SetIndexBuffer(IB);
		VA->AddVertexBuffer(VB);

		WorldTransform.Reset();
	}

	void Mesh::Bind()
	{
		MeshShader->Bind(ShaderParameters);
		VA->Bind();
	}

	void Mesh::Unbind()
	{
        MeshShader->Unbind(ShaderParameters);
	}

	void Mesh::SetShaderParam(ShaderParamType type, const GLchar* name, void* value)
	{
		ShaderParameters.push_back(new ShaderParam(type, name, value));
	}
}
