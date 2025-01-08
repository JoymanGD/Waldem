#include "wdpch.h"
#include "Mesh.h"

#include "Waldem/Application.h"

namespace Waldem
{
	Mesh::Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indexBufferData, uint32_t indexBufferDataSize, Material material, BoundingBox bBox)
	{
		VertexBuffer = Renderer::CreateBuffer(BufferType::VertexBuffer, vertexBufferData, vertexBufferDataSize);
		IndexBuffer = Renderer::CreateBuffer(BufferType::IndexBuffer, indexBufferData, indexBufferDataSize);
		BBox = bBox;

		MeshMaterial = material;

		ObjectMatrix = glm::identity<Matrix4>();
	}
}
