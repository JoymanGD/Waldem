#include "wdpch.h"
#include "Mesh.h"

namespace Waldem
{
	Mesh::Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indices, uint32_t indicesAmount, const BufferLayout& layout, Material material)
	{
		VB = VertexBuffer::Create(vertexBufferData, vertexBufferDataSize);
		VB->SetLayout(layout);
		IB = IndexBuffer::Create(indices, indicesAmount);
		IB->SetIndices(indices);

		MeshMaterial = material;
	}
}
