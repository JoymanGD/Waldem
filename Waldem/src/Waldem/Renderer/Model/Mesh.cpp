#include "wdpch.h"
#include "Mesh.h"
#include "Waldem/Application.h"

namespace Waldem
{
	Mesh::Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indexBufferData, uint32_t indexBufferDataSize, Material* material, BoundingBox bBox, String name, Matrix4 objectMatrix)
	{
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, vertexBufferData, vertexBufferDataSize, sizeof(Vertex));
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, indexBufferData, indexBufferDataSize, sizeof(uint32_t));
		BBox = bBox;
		Name = name;

		CurrentMaterial = material;

		ObjectMatrix = objectMatrix;
	}
}
