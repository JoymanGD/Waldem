#include "wdpch.h"
#include "StaticMesh.h"
#include "Waldem/Engine.h"
#include "Waldem/Utils/GeometryUtils.h"

namespace Waldem
{
	MeshBase::MeshBase(WArray<Vertex> vertexData, WArray<uint> indexData, Path materialPath, AABB bBox, WString name, Matrix4 objectMatrix)
	{
		MaterialPath = materialPath;
		VertexData = vertexData;
		IndexData = indexData;
		GeometryUtils::ExtractPositionsFromVertexData(VertexData, Positions);
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, VertexData.GetSize(), sizeof(Vertex), VertexData.GetData());
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, IndexData.GetSize(), sizeof(uint), IndexData.GetData());
		BBox = bBox;
		Name = name;
		ObjectMatrix = objectMatrix;
		Type = AssetType::Mesh;
	}

	void MeshBase::Serialize(WDataBuffer& outData)
	{
		VertexData.Serialize(outData);
		IndexData.Serialize(outData);
		outData << MaterialPath;
		outData << BBox;
		outData << ObjectMatrix;
	}

	void MeshBase::Deserialize(WDataBuffer& inData)
	{
		VertexData.Deserialize(inData);
		IndexData.Deserialize(inData);
		inData >> MaterialPath;
		inData >> BBox;
		inData >> ObjectMatrix;
		GeometryUtils::ExtractPositionsFromVertexData(VertexData, Positions);
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, VertexData.GetSize(), sizeof(Vertex), VertexData.GetData());
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, IndexData.GetSize(), sizeof(uint), IndexData.GetData());
	}
}
