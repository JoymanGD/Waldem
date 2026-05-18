#pragma once
#include "Material.h"
#include "StaticMesh.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Editor/AssetReference/MaterialReference.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
#define MAX_BONES_PER_VERTEX 4
    
    struct WALDEM_API VertexBones
    {
        int Joints[MAX_BONES_PER_VERTEX] = { -1, -1, -1, -1 };
        float Weights[MAX_BONES_PER_VERTEX] = { 0, 0, 0, 0 };
    };
    
    class WALDEM_API SkeletalMesh : public MeshBase
    {
    public:
        SkeletalMesh() {}
        SkeletalMesh(WString name) : MeshBase(name) {}
        
        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

        Buffer* VertexBonesBuffer = nullptr;
        WArray<VertexBones> VertexBonesDatas;
        int BoneCount = 0;
        WArray<Matrix4> InverseBindPoseMatrices;
    };
}