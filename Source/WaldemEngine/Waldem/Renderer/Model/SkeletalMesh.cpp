#include "wdpch.h"
#include "SkeletalMesh.h"

#include "Waldem/Engine.h"
#include "Waldem/Utils/GeometryUtils.h"

namespace Waldem
{
	void SkeletalMesh::Serialize(WDataBuffer& outData)
	{
		VertexData.Serialize(outData);
		IndexData.Serialize(outData);
		VertexBonesDatas.Serialize(outData);
		InverseBindPoseMatrices.Serialize(outData);
		outData << BoneCount;
		outData << MaterialPath;
		outData << BBox;
		outData << ObjectMatrix;
	}

	void SkeletalMesh::Deserialize(WDataBuffer& inData)
	{
		VertexData.Deserialize(inData);
		IndexData.Deserialize(inData);
		VertexBonesDatas.Deserialize(inData);
		InverseBindPoseMatrices.Deserialize(inData);
		inData >> BoneCount;
		inData >> MaterialPath;
		inData >> BBox;
		inData >> ObjectMatrix;
		GeometryUtils::ExtractPositionsFromVertexData(VertexData, Positions);
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, VertexData.GetSize(), sizeof(Vertex), VertexData.GetData());
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, IndexData.GetSize(), sizeof(uint), IndexData.GetData());
		VertexBonesBuffer = Renderer::CreateBuffer("VertexBonesBuffer", BufferType::StorageBuffer, VertexBonesDatas.GetSize(), sizeof(float) * MAX_BONES_PER_VERTEX + sizeof(int) * MAX_BONES_PER_VERTEX, VertexBonesDatas.GetData());
	}
}
