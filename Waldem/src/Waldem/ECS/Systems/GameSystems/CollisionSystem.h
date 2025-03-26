#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Model/Simplex.h"
#include "Waldem/Renderer/Model/Transform.h"
#include "Waldem/Utils/GeometryUtils.h"
#include "Waldem/Types/WMap.h"

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

        // Contact EPA(const Simplex& simplex, const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform& worldTransformA, Transform& worldTransformB)
        // {
        //     WArray<Vector3> polytope;
        //     for (auto point : simplex)
        //     {
        //         polytope.Add(point);
        //     }
        //     
        //     WArray<size_t> faces =
        //     {
        //         0, 1, 2,
        //         0, 3, 1,
        //         0, 2, 3,
        //         1, 3, 2
        //     };
        //     
        //     auto [normals, minFace] = GeometryUtils::GetFaceNormals(polytope, faces);
        //
        //     Vector3 minNormal;
        //     float minDistance = FLT_MAX;
	       //
        //     while (minDistance == FLT_MAX)
        //     {
        //         minNormal = normals[minFace];
        //         minDistance = normals[minFace].w;
        //
        //         Vector3 support = GeometryUtils::Support(colliderA, colliderB, worldTransformA, worldTransformB, minNormal);
        //         float sDistance = dot(minNormal, support);
        //
        //         if (abs(sDistance - minDistance) > 0.001f)
        //         {
        //             minDistance = FLT_MAX;
        //             
        //             WMap<size_t, size_t> uniqueEdges;
        //
        //             for (size_t i = 0; i < normals.Num(); i++)
        //             {
        //                 if (GeometryUtils::SameDirection(normals[i], support))
        //                 {
        //                     size_t f = i * 3;
        //
        //                     GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f,     f + 1);
        //                     GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
        //                     GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 2, f    );
        //
        //                     faces[f + 2] = faces.Last(); faces.RemoveLast();
        //                     faces[f + 1] = faces.Last(); faces.RemoveLast();
        //                     faces[f] = faces.Last(); faces.RemoveLast();
        //
        //                     normals[i] = normals.Last(); // pop-erase
        //                     normals.RemoveLast();
        //
        //                     i--;
        //                 }
        //             }
        //
        //             WArray<size_t> newFaces;
        //             for (auto [edgeIndex1, edgeIndex2] : uniqueEdges)
        //             {
        //                 newFaces.Add(edgeIndex1);
        //                 newFaces.Add(edgeIndex2);
        //                 newFaces.Add(polytope.Num());
        //             }
			     //
        //             polytope.Add(support);
        //
        //             auto [newNormals, newMinFace] = GeometryUtils::GetFaceNormals(polytope, newFaces);
        //
        //             float oldMinDistance = FLT_MAX;
        //             for (size_t i = 0; i < normals.Num(); i++)
        //             {
        //                 if (normals[i].w < oldMinDistance)
        //                 {
        //                     oldMinDistance = normals[i].w;
        //                     minFace = i;
        //                 }
        //             }
        //
        //             if (newNormals[newMinFace].w < oldMinDistance)
        //             {
        //                 minFace = newMinFace + normals.Num();
        //             }
        //
        //             faces.AddRange(newFaces);
        //             normals.AddRange(newNormals);
        //         }
        //     }
        //
        //     Contact points;
        //
        //     points.Normal = minNormal;
        //     points.PenetrationDepth = minDistance + 0.001f;
        //     points.HasCollision = true;
        //
        //     return points;
        // }

        Contact EPA(const Simplex& simplex, const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB)
        {
            Contact contact;
            
            contact.HasCollision = false;

            std::vector<Vector3> polytope(simplex.begin(), simplex.end());
            std::vector<size_t> faces =
            {
                0, 1, 2,
                0, 3, 1,
                0, 2, 3,
                1, 3, 2
            };

            // list: vec4(normal, distance), index: min distance
            auto [normals, minFace] = GeometryUtils::GetFaceNormals(polytope, faces);

            Vector3  minNormal;
            float minDistance = FLT_MAX;

            Vector3 lastSupportA, lastSupportB;
	
            while (minDistance == FLT_MAX)
            {
                minNormal   = normals[minFace];
                minDistance = normals[minFace].w;
 
                Vector3 supportA = colliderA->FindFurthestPoint(minNormal, worldTransformA);
                Vector3 supportB = colliderB->FindFurthestPoint(-minNormal, worldTransformB);
                
                Vector3 support = supportA - supportB;
                
                float sDistance = dot(minNormal, support);
 
                if (abs(sDistance - minDistance) > 0.01f)
                {
                    minDistance = FLT_MAX;

                    std::vector<std::pair<size_t, size_t>> uniqueEdges;

                    for (size_t i = 0; i < normals.size(); i++)
                    {                        
                        if (GeometryUtils::SameDirection(normals[i], support-polytope[faces[i*3]]))
                        {
                            size_t f = i * 3;

                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 2, f);

                            faces[f + 2] = faces.back(); faces.pop_back();
                            faces[f + 1] = faces.back(); faces.pop_back();
                            faces[f] = faces.back(); faces.pop_back();

                            normals[i] = normals.back(); // pop-erase
                            normals.pop_back();

                            i--;
                        }
                    }

                    std::vector<size_t> newFaces;
                    for (auto [edgeIndex1, edgeIndex2] : uniqueEdges)
                    {
                        newFaces.push_back(edgeIndex1);
                        newFaces.push_back(edgeIndex2);
                        newFaces.push_back(polytope.size());
                    }
			 
                    polytope.push_back(support);

                    auto [newNormals, newMinFace] = GeometryUtils::GetFaceNormals(polytope, newFaces);

                    float oldMinDistance = FLT_MAX;
                    for (size_t i = 0; i < normals.size(); i++)
                    {
                        if (normals[i].w < oldMinDistance)
                        {
                            oldMinDistance = normals[i].w;
                            minFace = i;
                        }
                    }

                    if(newNormals.empty())
                        return contact;
                    
                    if (newNormals[newMinFace].w < oldMinDistance)
                    {
                        minFace = newMinFace + normals.size();
                    }
 
                    faces.insert(faces.end(), newFaces.begin(), newFaces.end());
                    normals.insert(normals.end(), newNormals.begin(), newNormals.end());
                }

                lastSupportA = supportA;
                lastSupportB = supportB;
            }

            contact.ContactPointA = lastSupportA;
            contact.ContactPointB = lastSupportB;
            contact.Normal = minNormal;
            contact.PenetrationDepth = minDistance + 0.001f;
            contact.HasCollision = true;
 
            return contact;
        }

        Contact GJK(const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB)
        {
            Contact contact;
            
            Vector3 support = GeometryUtils::Support(colliderA, colliderB, worldTransformA, worldTransformB, Vector3(1,0,0));

            Simplex points;
            points.Add(support);

            Vector3 direction = -support;

            while(true)
            {
                support = GeometryUtils::Support(colliderA, colliderB, worldTransformA, worldTransformB, direction);

                if(dot(support, direction) <= 0)
                {
                    contact.HasCollision = false;
                    return contact;
                }

                points.Add(support);

                if(GeometryUtils::NextSimplex(points, direction))
                {
                    contact = EPA(points, colliderA, colliderB, worldTransformA, worldTransformB);
                    
                    return contact;
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

        void NarrowPhaseCollision(WArray<CollisionPair>& collisions, WArray<ColliderComponent*>& colliders, WArray<RigidBody*>& rigidBodies, WArray<Transform*>& transforms)
        {
            for (auto collision : collisions)
            {
                ColliderComponent* collider1 = colliders[collision.first];
                ColliderComponent* collider2 = colliders[collision.second];
                Transform* worldTransformA = transforms[collision.first];
                Transform* worldTransformB = transforms[collision.second];
                RigidBody* rigidBodyA = rigidBodies[collision.first];
                RigidBody* rigidBodyB = rigidBodies[collision.second];

                auto contact = GJK(collider1, collider2, worldTransformA, worldTransformB);
                
                if (contact.HasCollision)
                {
                    collider1->IsColliding = true;
                    collider2->IsColliding = true;
                
                    ResolveCollisions(worldTransformA, worldTransformB, rigidBodyA, rigidBodyB, contact);
                }
            }
        }

        void ResolveCollisions(Transform* transformA, Transform* transformB, RigidBody* rigidBodyA, RigidBody* rigidBodyB, const Contact& contact)
        {
            Vector3 contactPoint = (contact.ContactPointA + contact.ContactPointB) * 0.5f;

            // Check kinematic state
            bool aDynamic = !rigidBodyA->IsKinematic;
            bool bDynamic = !rigidBodyB->IsKinematic;

            if (!aDynamic && !bDynamic) return;

            float totalInvMass = (aDynamic ? rigidBodyA->InvMass : 0.0f) + (bDynamic ? rigidBodyB->InvMass : 0.0f);
            if (totalInvMass == 0.0f) return;

            // Positional Correction
            Vector3 correction = (contact.PenetrationDepth / totalInvMass) * contact.Normal * 0.8f;

            if (aDynamic)
                transformA->Position -= correction * rigidBodyA->InvMass;
            if (bDynamic)
                transformB->Position += correction * rigidBodyB->InvMass;

            // Impulse-based response
            Vector3 ra = contactPoint - transformA->Position;
            Vector3 rb = contactPoint - transformB->Position;

            Vector3 relativeVel = (rigidBodyB->Velocity + cross(rigidBodyB->AngularVelocity, rb)) 
                                - (rigidBodyA->Velocity + cross(rigidBodyA->AngularVelocity, ra));

            float velAlongNormal = dot(relativeVel, contact.Normal);
            if (velAlongNormal > 0) return;

            float invMassSum = (aDynamic ? rigidBodyA->InvMass : 0.0f) + (bDynamic ? rigidBodyB->InvMass : 0.0f) +
                dot(contact.Normal,
                    (aDynamic ? cross(rigidBodyA->InvInertiaTensor * cross(ra, contact.Normal), ra) : Vector3(0.0f)) +
                    (bDynamic ? cross(rigidBodyB->InvInertiaTensor * cross(rb, contact.Normal), rb) : Vector3(0.0f)));

            float restitution = (rigidBodyA->Bounciness + rigidBodyB->Bounciness) * 0.5f;
            float j = -(1 + restitution) * velAlongNormal / invMassSum;
            Vector3 impulse = contact.Normal * j;

            if (aDynamic)
            {
                rigidBodyA->Velocity -= impulse * rigidBodyA->InvMass;
                rigidBodyA->AngularVelocity -= rigidBodyA->InvInertiaTensor * cross(ra, impulse);
            }

            if (bDynamic)
            {
                rigidBodyB->Velocity += impulse * rigidBodyB->InvMass;
                rigidBodyB->AngularVelocity += rigidBodyB->InvInertiaTensor * cross(rb, impulse);
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
            WArray<Transform*> transforms;
            WArray<ColliderComponent*> colliders;
            WArray<RigidBody*> rigidBodies;

            for (auto [transformEntity, transform, collider, rigidBody, meshComponent] : ECSManager->EntitiesWith<Transform, ColliderComponent, RigidBody, MeshComponent>())
            {
                boundingBoxes.Add(meshComponent.Mesh->BBox.GetTransformed(transform));
                collider.IsColliding = false;
                colliders.Add(&collider);
                rigidBodies.Add(&rigidBody);
                transforms.Add(&transform);
            }

            if(boundingBoxes.IsEmpty())
            {
                return;
            }

            UpdateBVH(RootNode, boundingBoxes);
            
            WArray<CollisionPair> collisions;

            BroadPhaseCollision(RootNode, RootNode, collisions);
            NarrowPhaseCollision(collisions, colliders, rigidBodies, transforms);
        }
    };
}
