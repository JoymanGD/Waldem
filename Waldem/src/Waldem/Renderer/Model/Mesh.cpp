#include "wdpch.h"
#include "Mesh.h"
#include "Waldem/Application.h"

namespace Waldem
{
	CMesh::CMesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indexBufferData, uint32_t indexBufferDataSize, WArray<Vector3> positions, Material* material, AABB bBox, String name, Matrix4 objectMatrix)
	{
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, vertexBufferData, vertexBufferDataSize, sizeof(Vertex));
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, indexBufferData, indexBufferDataSize, sizeof(uint32_t));
		BBox = bBox;
		Name = name;

		CurrentMaterial = material;

		ObjectMatrix = objectMatrix;

		Positions = positions;
	}
}
