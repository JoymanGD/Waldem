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
        int MaxIterations = 20;
        const float EPA_EPSILON = 1e-5;
        
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

        Contact EPA(const Simplex& simplex, const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB)
        {
            Contact contact;
            contact.HasCollision = false;

            std::vector<Vector3> polytope;
            std::vector<Vector3> sourcePointsA;
            std::vector<Vector3> sourcePointsB;

            for (auto& vertex : simplex)
            {
                polytope.push_back(vertex.Point);
                sourcePointsA.push_back(vertex.SupportA); 
                sourcePointsB.push_back(vertex.SupportB);
            }

            std::vector<size_t> faces = {
                0, 1, 2,
                0, 3, 1,
                0, 2, 3,
                1, 3, 2
            };

            auto [normals, minFace] = GeometryUtils::GetFaceNormals(polytope, faces);

            Vector3 minNormal;
            float minDistance = FLT_MAX;

            // We'll store the final 'face indices'
            int finalFaceIdx = -1;

            // Store expansions in parallel arrays
            // so each polytope vertex has a corresponding A/B source
            auto pushSupport = [&](const Vector3& v, const Vector3& a, const Vector3& b)
            {
                polytope.push_back(v);
                sourcePointsA.push_back(a);
                sourcePointsB.push_back(b);
            };

            // Iteration loop
            while (minDistance == FLT_MAX)
            {
                minNormal   = normals[minFace];
                minDistance = normals[minFace].w;
                finalFaceIdx = (int)minFace;

                // get support from colliders
                Vector3 supportA = colliderA->FindFurthestPoint(minNormal, worldTransformA);
                Vector3 supportB = colliderB->FindFurthestPoint(-minNormal, worldTransformB);
                Vector3 support  = supportA - supportB;

                float sDistance = dot(minNormal, support);

                // Check if this actually improves
                if (fabs(sDistance - minDistance) > 0.0001f) // smaller threshold
                {
                    minDistance = FLT_MAX;

                    std::vector<std::pair<size_t, size_t>> uniqueEdges;

                    for (size_t i = 0; i < normals.size(); i++)
                    {
                        if (GeometryUtils::SameDirection(normals[i], support - polytope[faces[i*3]]))
                        {
                            size_t f = i * 3;

                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 2, f);

                            // remove face i
                            faces[f + 2] = faces.back(); faces.pop_back();
                            faces[f + 1] = faces.back(); faces.pop_back();
                            faces[f]     = faces.back(); faces.pop_back();

                            normals[i] = normals.back();
                            normals.pop_back();

                            i--;
                        }
                    }

                    // build new faces from uniqueEdges
                    std::vector<size_t> newFaces;
                    size_t newVertIndex = polytope.size();

                    pushSupport(support, supportA, supportB);

                    for (auto [edgeIndex1, edgeIndex2] : uniqueEdges)
                    {
                        newFaces.push_back(edgeIndex1);
                        newFaces.push_back(edgeIndex2);
                        newFaces.push_back(newVertIndex);
                    }

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

                    if (newNormals.empty())
                        return contact;

                    if (newNormals[newMinFace].w < oldMinDistance)
                    {
                        minFace = newMinFace + normals.size();
                    }

                    faces.insert(faces.end(), newFaces.begin(), newFaces.end());
                    normals.insert(normals.end(), newNormals.begin(), newNormals.end());
                }
            }

            // We can compute a better contact point:
            contact.HasCollision = true;
            contact.Normal = minNormal;

            // optional margin:
            float margin = 0.00001f;
            contact.PenetrationDepth = minDistance + margin;

            // Let's find the actual face indices
            size_t idx0 = faces[finalFaceIdx*3 + 0];
            size_t idx1 = faces[finalFaceIdx*3 + 1];
            size_t idx2 = faces[finalFaceIdx*3 + 2];

            Vector3 v0 = polytope[idx0];
            Vector3 v1 = polytope[idx1];
            Vector3 v2 = polytope[idx2];

            Vector3 a0 = sourcePointsA[idx0];
            Vector3 a1 = sourcePointsA[idx1];
            Vector3 a2 = sourcePointsA[idx2];

            Vector3 b0 = sourcePointsB[idx0];
            Vector3 b1 = sourcePointsB[idx1];
            Vector3 b2 = sourcePointsB[idx2];

            // Solve for barycentric coords alpha0, alpha1, alpha2 such that
            // alpha0+alpha1+alpha2=1 and alpha0*v0 + alpha1*v1 + alpha2*v2=0
            float alpha0, alpha1, alpha2;
            GeometryUtils::ComputeBarycentricForOriginInTriangle(v0, v1, v2, alpha0, alpha1, alpha2);

            // Then final contact points
            contact.ContactPointA = alpha0 * a0 + alpha1 * a1 + alpha2 * a2;
            contact.ContactPointB = alpha0 * b0 + alpha1 * b1 + alpha2 * b2;

            return contact;
        }

        Contact GJK(const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB)
        {
            Contact contact;

            Vector3 initialDir = Vector3(1, 0, 0);
            
            SimplexVertex support = SimplexVertex(colliderA->FindFurthestPoint(initialDir, worldTransformA), colliderB->FindFurthestPoint(-initialDir, worldTransformB));

            Simplex points;
            points.Add(support);

            Vector3 direction = -support.Point;

            int iterations = 0;
            
            while(true)
            {
                if(iterations > MaxIterations)
                {
                    contact.HasCollision = false;
                    return contact;
                }
                
                support = SimplexVertex(colliderA->FindFurthestPoint(direction, worldTransformA), colliderB->FindFurthestPoint(-direction, worldTransformB));

                if(dot(support.Point, direction) <= 0)
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

                iterations++;
            }
        }

        void ResolveCollision(Transform* transformA, Transform* transformB, RigidBody* rigidBodyA, RigidBody* rigidBodyB, const Contact& contact)
        {
            if (!contact.HasCollision)
                return;

            // 1. Positional correction (same approach)
            {
                const float percent = 0.8f;  // typically 20–80% correction
                const float slop = 0.01f;    // penetration slop

                // penetration depth - slop
                float penDepth = std::max(contact.PenetrationDepth - slop, 0.0f);
                Vector3 correction = contact.Normal * (penDepth / (rigidBodyA->InvMass + rigidBodyB->InvMass)) * percent;

                transformA->Position -= correction * rigidBodyA->InvMass;
                transformB->Position += correction * rigidBodyB->InvMass;
            }

            // 2. Early-out if objects are separating along the contact normal
            //    We'll do a more accurate velocityAlongNormal by considering relative angular velocity too
            Vector3 ra = (contact.ContactPointA - transformA->Position); // radius from A's center of mass to contact
            Vector3 rb = (contact.ContactPointB - transformB->Position); // radius from B's center of mass to contact

            // linear velocity at contact point from each body
            Vector3 velA = rigidBodyA->Velocity + cross(rigidBodyA->AngularVelocity, ra);
            Vector3 velB = rigidBodyB->Velocity + cross(rigidBodyB->AngularVelocity, rb);

            Vector3 relativeVelocity = velB - velA;
            float velocityAlongNormal = dot(relativeVelocity, contact.Normal);

            if (velocityAlongNormal > 0.0f)
                return; // they're moving apart, no impulse needed

            // 3. Compute the impulse scalar 'j'
            //    For 3D rigid bodies, the effective mass is more complicated than just (1/mA + 1/mB).
            //    We have to factor in inertia for angular response.

            // Restitution (bounciness)
            float e = std::min(rigidBodyA->Bounciness, rigidBodyB->Bounciness);

            // Inverse mass sum along normal:
            // 1) linear: InvMassA + InvMassB
            // 2) angular: (rA x n)^T * InvInertiaA * (rA x n) + same for B
            Vector3 rA_x_n = cross(ra, contact.Normal);
            Vector3 rB_x_n = cross(rb, contact.Normal);

            // (I^-1) is the inverse inertia tensor in world space (3x3). For a simple diag, keep it in matrix form or do direct math
            // We'll denote them as invInertiaWorldA and invInertiaWorldB

            Vector3 angularFactorA = rigidBodyA->InvInertiaTensor * rA_x_n;
            float angularTermA = dot(rA_x_n, angularFactorA);

            Vector3 angularFactorB = rigidBodyB->InvInertiaTensor * rB_x_n;
            float angularTermB = dot(rB_x_n, angularFactorB);

            float invMassSum = rigidBodyA->InvMass + rigidBodyB->InvMass + angularTermA + angularTermB;

            float j = -(1.0f + e) * velocityAlongNormal;
            j /= invMassSum;

            // 4. Compute actual impulse vector
            Vector3 impulse = j * contact.Normal;

            // 5. Apply impulse — both linear and angular
            // For linear:
            rigidBodyA->Velocity -= impulse * rigidBodyA->InvMass;
            rigidBodyB->Velocity += impulse * rigidBodyB->InvMass;

            // For angular:
            // angularVelocity += InvInertia * (r x impulse)
            Vector3 angularImpulseA = rigidBodyA->InvInertiaTensor * cross(ra, -impulse);
            Vector3 angularImpulseB = rigidBodyB->InvInertiaTensor * cross(rb,  impulse);

            rigidBodyA->AngularVelocity += angularImpulseA;
            rigidBodyB->AngularVelocity += angularImpulseB;

            // If you store inertia in local space, you'd convert the cross(...) result to local coords
            // and then multiply by invInertiaLocal, or you keep a world-space inverse inertia matrix.
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

                if(rigidBodyA->IsKinematic && rigidBodyB->IsKinematic)
                {
                    continue;
                }

                auto contact = GJK(collider1, collider2, worldTransformA, worldTransformB);
                
                if (contact.HasCollision)
                {
                    collider1->IsColliding = true;
                    collider2->IsColliding = true;
                
                    ResolveCollision(worldTransformA, worldTransformB, rigidBodyA, rigidBodyB, contact);
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
