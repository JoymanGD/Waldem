#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/PhysicsComponent.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    struct BVHNode
    {
        BoundingBox Box;
        BVHNode* Left;
        BVHNode* Right;
        int ObjectIndex;
        String DebugName;

        bool IsLeaf() const { return Left == nullptr && Right == nullptr; }
    };
    
    using CollisionPair = std::pair<int, int>;
    
    class WALDEM_API PhysicsSystem : ISystem
    {
        // Pipeline* CollisionBPPipeline = nullptr;
        // ComputeShader* CollisionBPComputeShader = nullptr;
        // RootSignature* CollisionBPSignature = nullptr;
        //
        // Pipeline* CollisionNPPipeline = nullptr;
        // ComputeShader* CollisionNPComputeShader = nullptr;
        // RootSignature* CollisionNPSignature = nullptr;
        //
        // uint MaxCollisions = 10000;
        // uint MaxObjectsPerThread = 3;

    private:
        BVHNode* BuildBVH(WArray<BoundingBox>& objects, WArray<String>& names, int start, int end)
        {
            BVHNode* node = new BVHNode();
            int objectCount = end - start;

            // Compute the bounding box for this node
            BoundingBox box = objects[start];
            for (int i = start + 1; i < end; i++)
            {
                box.Expand(objects[i]);
            }
            node->Box = box;
            node->DebugName = names[start];

            // Base case: leaf node
            if (objectCount == 1)
            {
                node->ObjectIndex = start;
                node->Left = node->Right = nullptr;
                return node;
            }

            // Determine the longest axis to split
            Vector3 size = box.Max - box.Min;
            int axis = 0;
            if (size.y > size.x && size.y > size.z) axis = 1;
            if (size.z > size.x && size.z > size.y) axis = 2;

            // Sort objects by the selected axis
            int mid = start + objectCount / 2;
            std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end,
                [axis](const BoundingBox& a, const BoundingBox& b)
                {
                    return a.Min[axis] < b.Min[axis];
                });

            // Build child nodes
            node->Left = BuildBVH(objects, names, start, mid);
            node->Right = BuildBVH(objects, names, mid, end);

            return node;
        }

        void NarrowPhaseCollision(BVHNode* node1, BVHNode* node2)
        {
            WD_CORE_INFO("NP collision detected: {0} and {1}", node1->ObjectIndex, node2->ObjectIndex);
        }
        
        void BroadPhaseCollision(BVHNode* node1, BVHNode* node2, WArray<CollisionPair>& collisions)
        {
            if (!node1->Box.Intersects(node2->Box))
            {
                return;
            }

            if(node1->IsLeaf() && node2->IsLeaf())
            {
                if(node1->ObjectIndex != node2->ObjectIndex)
                {
                    int minIndex = std::min(node1->ObjectIndex, node2->ObjectIndex);
                    int maxIndex = std::max(node1->ObjectIndex, node2->ObjectIndex);
                    if(!collisions.Contains({minIndex, maxIndex}))
                    {
                        collisions.Add({minIndex, maxIndex});
                    }
                    NarrowPhaseCollision(node1, node2);
                }
                return;
            }

            if(node1->IsLeaf())
            {
                BroadPhaseCollision(node1, node2->Left, collisions);
                BroadPhaseCollision(node1, node2->Right, collisions);
            }
            else if(node2->IsLeaf())
            {
                BroadPhaseCollision(node1->Left, node2, collisions);
                BroadPhaseCollision(node1->Right, node2, collisions);
            }
            else
            {
                BroadPhaseCollision(node1->Left, node2->Left, collisions);
                BroadPhaseCollision(node1->Left, node2->Right, collisions);
                BroadPhaseCollision(node1->Right, node2->Left, collisions);
                BroadPhaseCollision(node1->Right, node2->Right, collisions);
            }
        }
        
    public:
        PhysicsSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            WArray<BoundingBox> boundingBoxes;
            WArray<String> names;

            for (auto [transformEntity, transform, physicsComponent, meshComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox);
                names.Add(meshComponent.Mesh->Name);
            }
            
            BVHNode* rootNode = BuildBVH(boundingBoxes, names, 0, boundingBoxes.Num());
            WArray<CollisionPair> collisions;
            
            BroadPhaseCollision(rootNode, rootNode, collisions);

            // WArray<BVHNode> bvhNodes;
            // struct CollisionConstantsCB
            // {
            //     uint ComponentCount;
            //     uint MaxObjectsPerThread;
            // } shaderCBuffer;
            //
            // shaderCBuffer.ComponentCount = boundingBoxes.Num();
            // shaderCBuffer.MaxObjectsPerThread = MaxObjectsPerThread;
            //
            // WArray<Resource> CollisionResources;
            // CollisionResources.Add(Resource("AABBs", RTYPE_Buffer, boundingBoxes.GetData(), sizeof(BoundingBox), boundingBoxes.GetSize(), 0));
            // CollisionResources.Add(Resource("BVHNodes", RTYPE_RWBuffer, nullptr, sizeof(BVHNode), bvhNodes.GetSize(), 0));
            // CollisionResources.Add(Resource("ConstantBuffer", RTYPE_ConstantBuffer, &shaderCBuffer, sizeof(CollisionConstantsCB), 1, 0));
            // CollisionBPSignature = Renderer::CreateRootSignature(CollisionResources);
            // CollisionBPComputeShader = Renderer::LoadComputeShader("Physics/AABBBroadPhase");
            // CollisionBPPipeline = Renderer::CreateComputePipeline("CollisionBPPipeline", CollisionBPSignature, CollisionBPComputeShader);
        }

        void Update(float deltaTime) override
        {
            // WArray<Transform> transforms;
            // WArray<PhysicsComponent*> physicsComponents;
            //
            // for (auto [transformEntity, transform, physicsComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent>())
            // {
            //     physicsComponents.Add(&physicsComponent);
            // }
            //
            // //Clear buffers
            // //CollisionBPSignature->ClearResource("BVHNodes");
            //
            // //Quad drawing pass
            // Renderer::SetPipeline(CollisionBPPipeline);
            // Renderer::SetRootSignature(CollisionBPSignature);
            // Point3 numThreads = Renderer::GetNumThreadsPerGroup(CollisionBPComputeShader);
            // Point3 groupCount = Point3(glm::ceil(physicsComponents.Num() / (float)numThreads.x), 1, 1);
            // Renderer::Compute(groupCount);
        }
    };
}