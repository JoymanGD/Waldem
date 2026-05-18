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
		outData << GlobalInverseTransform;

		uint nodeCount = (uint)AnimationNodes.Num();
		outData << nodeCount;
		for (uint i = 0; i < nodeCount; ++i)
		{
			AnimationNodes[i].Name.Serialize(outData);
			outData << AnimationNodes[i].ParentIndex;
			outData << AnimationNodes[i].BoneIndex;
			outData << AnimationNodes[i].LocalTransform;
		}

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
		inData >> GlobalInverseTransform;

		uint nodeCount = 0;
		inData >> nodeCount;
		AnimationNodes.Resize(nodeCount);
		for (uint i = 0; i < nodeCount; ++i)
		{
			AnimationNodes[i].Name.Deserialize(inData);
			inData >> AnimationNodes[i].ParentIndex;
			inData >> AnimationNodes[i].BoneIndex;
			inData >> AnimationNodes[i].LocalTransform;
		}

		inData >> MaterialPath;
		inData >> BBox;
		inData >> ObjectMatrix;
		GeometryUtils::ExtractPositionsFromVertexData(VertexData, Positions);
		VertexBuffer = Renderer::CreateBuffer("MeshVertexBuffer", BufferType::VertexBuffer, VertexData.GetSize(), sizeof(Vertex), VertexData.GetData());
		IndexBuffer = Renderer::CreateBuffer("MeshIndexBuffer", BufferType::IndexBuffer, IndexData.GetSize(), sizeof(uint), IndexData.GetData());
		VertexBonesBuffer = Renderer::CreateBuffer("VertexBonesBuffer", BufferType::StorageBuffer, VertexBonesDatas.GetSize(), sizeof(float) * MAX_BONES_PER_VERTEX + sizeof(int) * MAX_BONES_PER_VERTEX, VertexBonesDatas.GetData());
	}
}
