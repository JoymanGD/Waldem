#pragma once
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/AnimatorComponent.h"
#include "Waldem/ECS/Components/SkeletalMeshComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Animation/AnimationClip.h"
#include "Waldem/Time.h"
#include "Waldem/Editor/EditorSimulation.h"

namespace Waldem
{
    class WALDEM_API AnimationSystem : public ICoreSystem
    {
    public:
        void Initialize() override
        {
            ECS::World.system<AnimatorComponent, SkeletalMeshComponent>("AnimationSystem").kind<ECS::OnDraw>().each([this](AnimatorComponent& animator, SkeletalMeshComponent& skeletalMeshComp)
            {
                if (!EditorSimulation::ShouldRunRuntimeSystems())
                    return;

                if(!IsActive)
                {
                    return;
                }

                if (!animator.Started)
                {
                    animator.Started = true;
                    animator.IsPlaying = animator.PlayOnStart ? 1 : 0;
                }

                if (!animator.IsPlaying)
                    return;

                if (!animator.ClipRef.IsValid())
                {
                    if (!animator.ClipRef.Reference.empty() && animator.ClipRef.Reference != "Empty")
                        animator.ClipRef.LoadAsset();
                    
                    if (!animator.ClipRef.IsValid())
                        return;
                }

                if (!skeletalMeshComp.MeshRef.IsValid() || !skeletalMeshComp.BoneMatricesBuffer)
                    return;

                const AnimationClip* clip = animator.ClipRef.Clip;
                const SkeletalMesh*  mesh = skeletalMeshComp.MeshRef.Mesh;
                int boneCount = mesh->BoneCount;

                if (boneCount == 0 || mesh->AnimationNodes.Num() == 0)
                    return;

                float ticksPerSec = clip->TicksPerSecond > 0.0f ? clip->TicksPerSecond : 25.0f;
                animator.CurrentTimeTicks += Time::DeltaTime * ticksPerSec * animator.PlaybackSpeed;

                if (animator.Loop)
                    animator.CurrentTimeTicks = fmodf(animator.CurrentTimeTicks, clip->DurationTicks);
                else
                    animator.CurrentTimeTicks = glm::min(animator.CurrentTimeTicks, clip->DurationTicks);

                const float t = animator.CurrentTimeTicks;
            
                int nodeCount = (int)mesh->AnimationNodes.Num();
                WArray<Matrix4> globalTransforms;
                globalTransforms.Resize(nodeCount, Matrix4(1.0f));
                WArray<Matrix4> finalMatrices;
                finalMatrices.Resize(boneCount, Matrix4(1.0f));

                for (int i = 0; i < nodeCount; ++i)
                {
                    const AnimationNode& node = mesh->AnimationNodes[i];

                    Matrix4 localTransform = node.LocalTransform;

                    int ch = clip->FindChannel(node.Name);
                    if (ch >= 0)
                    {
                        const AnimationChannel& chan = clip->Channels[ch];
                        Vector3 pos = SamplePosition(chan, t);
                        Quaternion rot = SampleRotation(chan, t);
                        Vector3 scale = SampleScale(chan, t);

                        localTransform = translate(Matrix4(1.0f), pos) * mat4_cast(rot) * glm::scale(Matrix4(1.0f), scale);
                    }

                    Matrix4 parentGlobal = node.ParentIndex >= 0 ? globalTransforms[node.ParentIndex] : Matrix4(1.0f);

                    globalTransforms[i] = parentGlobal * localTransform;

                    if (node.BoneIndex >= 0)
                    {
                        finalMatrices[node.BoneIndex] = globalTransforms[i] * mesh->InverseBindPoseMatrices[node.BoneIndex];
                    }
                }

                Renderer::UploadBuffer(skeletalMeshComp.BoneMatricesBuffer, finalMatrices.GetData(), (uint32_t)(boneCount * sizeof(Matrix4)));
            });
        }

    private:
        static Vector3 SamplePosition(const AnimationChannel& chan, float t)
        {
            uint n = (uint)chan.PositionKeys.Num();
            if (n == 0) return Vector3(0.0f);
            if (n == 1) return chan.PositionKeys[0].Value;
            for (uint i = 0; i + 1 < n; ++i)
            {
                if (t < chan.PositionKeys[i + 1].Time)
                {
                    float dt = chan.PositionKeys[i + 1].Time - chan.PositionKeys[i].Time;
                    float f  = (dt > 0.0f) ? (t - chan.PositionKeys[i].Time) / dt : 0.0f;
                    return mix(chan.PositionKeys[i].Value, chan.PositionKeys[i + 1].Value, f);
                }
            }
            return chan.PositionKeys.Last().Value;
        }

        static Quaternion SampleRotation(const AnimationChannel& chan, float t)
        {
            uint n = (uint)chan.RotationKeys.Num();
            if (n == 0) return Quaternion(1, 0, 0, 0);
            if (n == 1) return chan.RotationKeys[0].Value;
            for (uint i = 0; i + 1 < n; ++i)
            {
                if (t < chan.RotationKeys[i + 1].Time)
                {
                    float dt = chan.RotationKeys[i + 1].Time - chan.RotationKeys[i].Time;
                    float f  = (dt > 0.0f) ? (t - chan.RotationKeys[i].Time) / dt : 0.0f;
                    return normalize(slerp(chan.RotationKeys[i].Value, chan.RotationKeys[i + 1].Value, f));
                }
            }
            return chan.RotationKeys.Last().Value;
        }

        static Vector3 SampleScale(const AnimationChannel& chan, float t)
        {
            uint n = (uint)chan.ScaleKeys.Num();
            if (n == 0) return Vector3(1.0f);
            if (n == 1) return chan.ScaleKeys[0].Value;
            for (uint i = 0; i + 1 < n; ++i)
            {
                if (t < chan.ScaleKeys[i + 1].Time)
                {
                    float dt = chan.ScaleKeys[i + 1].Time - chan.ScaleKeys[i].Time;
                    float f  = (dt > 0.0f) ? (t - chan.ScaleKeys[i].Time) / dt : 0.0f;
                    return mix(chan.ScaleKeys[i].Value, chan.ScaleKeys[i + 1].Value, f);
                }
            }
            return chan.ScaleKeys.Last().Value;
        }
    };
}
