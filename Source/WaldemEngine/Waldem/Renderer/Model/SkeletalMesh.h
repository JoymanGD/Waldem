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

    // Node in the full scene sub-tree used for FK evaluation.
    // Includes both bone nodes and non-bone ancestor nodes (e.g. "Armature").
    // Stored in DFS pre-order so ParentIndex < own index — parents are always processed first.
    struct WALDEM_API AnimationNode
    {
        WString Name;
        int     ParentIndex   = -1; // index into AnimationNodes, -1 = scene-relative root
        int     BoneIndex     = -1; // index into InverseBindPoseMatrices, -1 if not a bone
        Matrix4 LocalTransform = Matrix4(1.0f);
    };

    struct WALDEM_API VertexBones
    {
        int   Joints[MAX_BONES_PER_VERTEX]  = { -1, -1, -1, -1 };
        float Weights[MAX_BONES_PER_VERTEX] = {  0,  0,  0,  0 };
    };

    class WALDEM_API SkeletalMesh : public MeshBase
    {
    public:
        SkeletalMesh() {}
        SkeletalMesh(WString name) : MeshBase(name) {}

        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

        Buffer* VertexBonesBuffer = nullptr;
        WArray<VertexBones>    VertexBonesDatas;
        int                    BoneCount = 0;
        WArray<Matrix4>        InverseBindPoseMatrices; // one per bone (indexed by BoneIndex)
        WArray<AnimationNode>  AnimationNodes;          // full sub-tree for FK, DFS pre-order
        Matrix4                GlobalInverseTransform = Matrix4(1.0f); // inverse(scene root transform)
    };
}
