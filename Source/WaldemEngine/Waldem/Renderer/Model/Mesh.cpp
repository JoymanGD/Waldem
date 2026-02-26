#include "wdpch.h"
#include "Mesh.h"
#include "Waldem/Engine.h"

namespace Waldem
{
	void ExtractPositionsFromVertexData(const WArray<Vertex>& vertexData, WArray<Vector3>& positions)
	{
		positions.Resize(vertexData.Num());
		for (size_t i = 0; i < vertexData.Num(); ++i)
		{
			positions[i] = vertexData[i].Position;
		}
	}
	
	CMesh::CMesh(WArray<Vertex> vertexData, WArray<uint> indexData, MaterialReference materialRef, AABB bBox, WString name, Matrix4 objectMatrix)
	{
		VertexData = vertexData;
		IndexData = indexData;
		ExtractPositionsFromVertexData(VertexData, Positions);
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, VertexData.GetSize(), sizeof(Vertex), VertexData.GetData());
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, IndexData.GetSize(), sizeof(uint), IndexData.GetData());
		BBox = bBox;
		Name = name;
		MaterialRef = materialRef;
		ObjectMatrix = objectMatrix;
		Type = AssetType::Mesh;
	}

	void CMesh::Serialize(WDataBuffer& outData)
	{
		VertexData.Serialize(outData);
		IndexData.Serialize(outData);
		outData << MaterialRef.Reference;
		outData << BBox;
		outData << ObjectMatrix;
	}

	void CMesh::Deserialize(WDataBuffer& inData)
	{
		VertexData.Deserialize(inData);
		IndexData.Deserialize(inData);
		inData >> MaterialRef.Reference;
		inData >> BBox;
		inData >> ObjectMatrix;
		ExtractPositionsFromVertexData(VertexData, Positions);
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, VertexData.GetSize(), sizeof(Vertex), VertexData.GetData());
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, IndexData.GetSize(), sizeof(uint), IndexData.GetData());

		if(MaterialRef.Reference != "Empty")
		{
			MaterialRef.LoadAsset();
		}
	}
}
