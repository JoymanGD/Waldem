#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Model/Simplex.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/Utils/GeometryUtils.h"

namespace Waldem
{
    class WALDEM_API CollisionSystem : ISystem
    {
    private:
        BVHNode* RootNode;
        
        BVHNode* BuildBVH(WArray<AABB>& objects, WArray<String>& names, int start, int end)
        {
            BVHNode* node = new BVHNode();
            int objectCount = end - start;

            AABB box = objects[start];
            for (int i = start + 1; i < end; i++)
            {
                box.Expand(objects[i]);
            }
            node->Box = box;
            node->DebugName = names[start];

            if (objectCount == 1)
            {
                node->ObjectIndex = start;
                node->Left = node->Right = nullptr;
                return node;
            }

            Vector3 size = box.Max - box.Min;
            int axis = 0;
            if (size.y > size.x && size.y > size.z) axis = 1;
            if (size.z > size.x && size.z > size.y) axis = 2;

            int mid = start + objectCount / 2;
            std::nth_element(objects.begin() + start, objects.begin() + mid, objects.begin() + end,
                [axis](const AABB& a, const AABB& b)
                {
                    return a.Min[axis] < b.Min[axis];
                });

            node->Left = BuildBVH(objects, names, start, mid);
            node->Right = BuildBVH(objects, names, mid, end);

            return node;
        }

        void UpdateBVH(BVHNode* node, WArray<AABB>& objects)
        {
            if (node->IsLeaf()) 
            {
                node->Box = objects[node->ObjectIndex];
                return;
            }

            if (node->Left) UpdateBVH(node->Left, objects);
            if (node->Right) UpdateBVH(node->Right, objects);

            node->Box = node->Left ? node->Left->Box : AABB();
            if (node->Right) node->Box.Expand(node->Right->Box);
        }

        bool GJK(const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform& worldTransformA, Transform& worldTransformB)
        {
            Vector3 support = GeometryUtils::Support(colliderA, colliderB, worldTransformA, worldTransformB, Vector3(1,0,0));

            Simplex points;
            points.Add(support);

            Vector3 direction = -support;

            while(true)
            {
                support = GeometryUtils::Support(colliderA, colliderB, worldTransformA, worldTransformB, direction);

                if(dot(support, direction) <= 0)
                {
                    return false;
                }

                points.Add(support);

                if(GeometryUtils::NextSimplex(points, direction))
                {
                    return true;
                }
            }
        }

        void BroadPhaseCollision(BVHNode* node1, BVHNode* node2, WArray<CollisionPair>& collisions)
        {
            if (!node1->Box.Intersects(node2->Box))
            {
                return;
            }

            if (node1->IsLeaf() && node2->IsLeaf())
            {
                if (node1->ObjectIndex != node2->ObjectIndex)
                {
                    int minIndex = std::min(node1->ObjectIndex, node2->ObjectIndex);
                    int maxIndex = std::max(node1->ObjectIndex, node2->ObjectIndex);

                    std::pair<int, int> pair = {minIndex, maxIndex};
                    if (std::find(collisions.begin(), collisions.end(), pair) == collisions.end())
                    {
                        collisions.Add(pair);
                    }
                }
                return;
            }

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
                BroadPhaseCollision(node1->Left, node2->Left, collisions);
                BroadPhaseCollision(node1->Left, node2->Right, collisions);
                BroadPhaseCollision(node1->Right, node2->Left, collisions);
                BroadPhaseCollision(node1->Right, node2->Right, collisions);
            }
        }

        void NarrowPhaseCollision(WArray<CollisionPair>& collisions, WArray<ColliderComponent*>& colliders, WArray<Transform>& transforms)
        {
            for (auto collision : collisions)
            {
                ColliderComponent* collider1 = colliders[collision.first];
                ColliderComponent* collider2 = colliders[collision.second];
                Transform& worldTransformA = transforms[collision.first];
                Transform& worldTransformB = transforms[collision.second];
                
                if (collider1->Type == WD_COLLIDER_TYPE_MESH && collider2->Type == WD_COLLIDER_TYPE_MESH)
                {
                    if (GJK(collider1, collider2, worldTransformA, worldTransformB))
                    {
                        // WD_CORE_INFO("Collision detected between {0} and {1}", collider1->MeshData.Mesh.Name, collider2->MeshData.Mesh.Name);
                        collider1->IsColliding = true;
                        collider2->IsColliding = true;
                    }
                }
            }
        }
        
    public:
        CollisionSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            WArray<AABB> boundingBoxes;
            WArray<String> names;

            for (auto [transformEntity, transform, collider, meshComponent] : ECSManager->EntitiesWith<Transform, ColliderComponent, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox.GetTransformed(transform));
                names.Add(meshComponent.Mesh->Name);
            }

            if(boundingBoxes.IsEmpty())
            {
                return;
            }
            
            RootNode = BuildBVH(boundingBoxes, names, 0, boundingBoxes.Num());
        }

        void Update(float deltaTime) override
        {
            WArray<AABB> boundingBoxes;
            WArray<Transform> transforms;
            WArray<ColliderComponent*> colliders;

            for (auto [transformEntity, transform, collider, meshComponent] : ECSManager->EntitiesWith<Transform, ColliderComponent, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox.GetTransformed(transform));
                collider.IsColliding = false;
                colliders.Add(&collider);
                transforms.Add(transform);
            }

            if(boundingBoxes.IsEmpty())
            {
                return;
            }

            UpdateBVH(RootNode, boundingBoxes);
            
            WArray<CollisionPair> collisions;

            BroadPhaseCollision(RootNode, RootNode, collisions);
            NarrowPhaseCollision(collisions, colliders, transforms);
        }
    };
}
