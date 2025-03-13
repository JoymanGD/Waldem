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
        BVHNode* RootNode;

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

        void UpdateBVH(BVHNode* node, WArray<BoundingBox>& objects)
        {
            // Leaf node: update the bounding box from the object directly
            if (node->IsLeaf()) 
            {
                node->Box = objects[node->ObjectIndex];
                return;
            }

            // Update child nodes
            if (node->Left) UpdateBVH(node->Left, objects);
            if (node->Right) UpdateBVH(node->Right, objects);

            // Recompute the bounding box to encompass both children
            node->Box = node->Left ? node->Left->Box : BoundingBox();
            if (node->Right) node->Box.Expand(node->Right->Box);
        }

        void NarrowPhaseCollision(BVHNode* node1, BVHNode* node2)
        {
            WD_CORE_INFO("NP collision detected: {0} and {1}", node1->ObjectIndex, node2->ObjectIndex);
        }

        struct CollisionPairHash
        {
            size_t operator()(const std::pair<int, int>& p) const
            {
                return std::hash<int>()(p.first) ^ std::hash<int>()(p.second);
            }
        };

        // void BroadPhaseCollision(BVHNode* node1, BVHNode* node2, std::unordered_set<std::pair<int, int>, CollisionPairHash>& collisions)
        // {
        //     // Null check to ensure nodes are valid
        //     if (node1 == nullptr || node2 == nullptr)
        //         return;
        //     
        //     // Early exit if bounding boxes do not intersect
        //     if (!node1->Box.Intersects(node2->Box))
        //         return;
        //
        //     // If both nodes are leaves, check for collision
        //     if (node1->IsLeaf() && node2->IsLeaf())
        //     {
        //         if (node1->ObjectIndex != node2->ObjectIndex)
        //         {
        //             int minIndex = std::min(node1->ObjectIndex, node2->ObjectIndex);
        //             int maxIndex = std::max(node1->ObjectIndex, node2->ObjectIndex);
        //             std::pair<int, int> pair = {minIndex, maxIndex};
        //
        //             // Insert the collision pair directly
        //             collisions.insert(pair);
        //         }
        //         return;
        //     }
        //
        //     // Prioritize traversal based on surface area (if available)
        //     float areaLeftRight = 0;
        //     float areaRightLeft = 0;
        //
        //     if (!node1->IsLeaf() && !node2->IsLeaf())
        //     {
        //         areaLeftRight = node1->Left->Box.SurfaceArea() + node2->Right->Box.SurfaceArea();
        //         areaRightLeft = node1->Right->Box.SurfaceArea() + node2->Left->Box.SurfaceArea();
        //     }
        //
        //     // Check the best combination first (more likely to intersect)
        //     if (areaLeftRight < areaRightLeft)
        //     {
        //         BroadPhaseCollision(node1->Left, node2->Left, collisions);
        //         BroadPhaseCollision(node1->Left, node2->Right, collisions);
        //         BroadPhaseCollision(node1->Right, node2->Left, collisions);
        //         BroadPhaseCollision(node1->Right, node2->Right, collisions);
        //     }
        //     else
        //     {
        //         BroadPhaseCollision(node1->Right, node2->Left, collisions);
        //         BroadPhaseCollision(node1->Right, node2->Right, collisions);
        //         BroadPhaseCollision(node1->Left, node2->Left, collisions);
        //         BroadPhaseCollision(node1->Left, node2->Right, collisions);
        //     }
        // }

        void BroadPhaseCollision(BVHNode* node1, BVHNode* node2, WArray<CollisionPair>& collisions)
        {
            // Perform the bounding box intersection check first
            if (!node1->Box.Intersects(node2->Box))
            {
                return; // No need to continue if the boxes don't intersect
            }

            // If both nodes are leaf nodes, check for collisions between the objects they contain
            if (node1->IsLeaf() && node2->IsLeaf())
            {
                if (node1->ObjectIndex != node2->ObjectIndex)
                {
                    int minIndex = std::min(node1->ObjectIndex, node2->ObjectIndex);
                    int maxIndex = std::max(node1->ObjectIndex, node2->ObjectIndex);

                    std::pair<int, int> pair = {minIndex, maxIndex}; // Store the pair of objects
                    if (std::find(collisions.begin(), collisions.end(), pair) == collisions.end()) // Avoid duplicate collisions
                    {
                        collisions.Add(pair);
                    }
                }
                return;
            }

            // If one of the nodes is a leaf, check the children of the other node
            if (node1->IsLeaf())
            {
                BroadPhaseCollision(node1, node2->Left, collisions);
                BroadPhaseCollision(node1, node2->Right, collisions);
            }
            else if (node2->IsLeaf())
            {
                BroadPhaseCollision(node1->Left, node2, collisions);
                BroadPhaseCollision(node1->Right, node2, collisions);
            }
            else
            {
                // If both nodes are internal, recursively check their children without prioritizing based on SAH
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
            WArray<PhysicsComponent*> physicsComponents;

            for (auto [transformEntity, transform, physicsComponent, meshComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox);
                names.Add(meshComponent.Mesh->Name);
                physicsComponents.Add(&physicsComponent);
            }
            
            RootNode = BuildBVH(boundingBoxes, names, 0, boundingBoxes.Num());
        }

        void Update(float deltaTime) override
        {
            WArray<BoundingBox> boundingBoxes;
            WArray<String> names;
            WArray<PhysicsComponent*> physicsComponents;

            for (auto [transformEntity, transform, physicsComponent, meshComponent] : ECSManager->EntitiesWith<Transform, PhysicsComponent, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox.GetTransformed(transform));
                names.Add(meshComponent.Mesh->Name);
                physicsComponents.Add(&physicsComponent);
            }

            UpdateBVH(RootNode, boundingBoxes);
            
            // std::unordered_set<std::pair<int, int>, CollisionPairHash> collisions;

            WArray<CollisionPair> collisions;
            BroadPhaseCollision(RootNode, RootNode, collisions);

            for (auto physicsComponent : physicsComponents)
            {
                physicsComponent->IsColliding = false;
            }

            for (auto collision : collisions)
            {
                physicsComponents[collision.first]->IsColliding = true;
                physicsComponents[collision.second]->IsColliding = true;
            }
        }
    };
}