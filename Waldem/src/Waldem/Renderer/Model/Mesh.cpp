#include "wdpch.h"
#include "Mesh.h"

#include "Waldem/Application.h"

namespace Waldem
{
	Mesh::Mesh(void* vertexBufferData, uint32_t vertexBufferDataSize, uint32_t* indices, uint32_t indicesAmount, Material material, BoundingBox bBox)
	{
		VB = Renderer::CreateVertexBuffer(vertexBufferData, vertexBufferDataSize);
		IB = Renderer::CreateIndexBuffer(indices, indicesAmount);
		IB->SetIndices(indices);
		BBox = bBox;

		MeshMaterial = material;
	}
}
