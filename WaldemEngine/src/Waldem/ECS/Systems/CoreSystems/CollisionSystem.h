#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Model/Plane.h"
#include "Waldem/Renderer/Model/Simplex.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Utils/GeometryUtils.h"

#define EPA_EPSILON 0.0001f
#define CACHE_CONTACT_POINT_DISTANCE 0.01f

namespace Waldem
{
    struct Collision
    {
        CollisionPair Colliders;
        Contact ContactData;
    };
    
    class WALDEM_API CollisionSystem : public ICoreSystem
    {
        BVHNode* RootNode = nullptr;
        int MaxIterations = 50;
        WArray<PersistentContact> ContactCache;
        WArray<ECS::Entity> Entities;
        
        BVHNode* BuildBVH(int start, int end)
        {
            BVHNode* node = new BVHNode();
            int objectCount = end - start;

            auto& transform = Entities[start].get_mut<Transform>();
            AABB box = Entities[start].get_mut<AABB>().GetTransformed(transform);
            for (int i = start + 1; i < end; i++)
            {
                auto& secondTransform = Entities[i].get_mut<Transform>();
                box.Expand(Entities[i].get_mut<AABB>().GetTransformed(secondTransform));
            }
            node->Box = box;
            node->DebugName = WString(Entities[start].name().c_str());

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
            std::nth_element(Entities.begin() + start, Entities.begin() + mid, Entities.begin() + end,
            [axis](const ECS::Entity& a, const ECS::Entity& b)
            {
                return a.get_mut<AABB>().Min[axis] < b.get_mut<AABB>().Min[axis];
            });

            node->Left = BuildBVH(start, mid);
            node->Right = BuildBVH(mid, end);

            return node;
        }

        void UpdateBVH(BVHNode* node)
        {
            if (node->IsLeaf()) 
            {
                auto& transform = Entities[node->ObjectIndex].get_mut<Transform>();
                node->Box = Entities[node->ObjectIndex].get_mut<AABB>().GetTransformed(transform);
                return;
            }

            if (node->Left) UpdateBVH(node->Left);
            if (node->Right) UpdateBVH(node->Right);

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

        Contact EPA(const Simplex& simplex, const ColliderComponent& colliderA, const ColliderComponent& colliderB, Transform& worldTransformA, Transform& worldTransformB)
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
        
            uint32_t iterations = 0;
            
            // Iteration loop
            while (minDistance == FLT_MAX && iterations < MaxIterations)
            {
                minNormal   = normals[minFace];
                minDistance = normals[minFace].w;
                finalFaceIdx = (int)minFace;
        
                // get support from colliders
                Vector3 supportA = colliderA.FindFurthestPoint(minNormal, worldTransformA);
                Vector3 supportB = colliderB.FindFurthestPoint(-minNormal, worldTransformB);
                Vector3 support  = supportA - supportB;
        
                float sDistance = dot(minNormal, support);
        
                // Check if this actually improves
                if (fabs(sDistance - minDistance) > EPA_EPSILON)
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
        
                iterations++;
            }
        
            // We can compute a better contact point:
            contact.HasCollision = true;
            
            contact.Normal = normalize(minNormal);
        
            // optional margin:
            contact.PenetrationDepth = minDistance;

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

        Contact GJK(const ColliderComponent& colliderA, const ColliderComponent& colliderB, Transform& worldTransformA, Transform& worldTransformB)
        {
            Contact contact;
        
            Vector3 initialDir = Vector3(1, 0, 0);
            
            SimplexVertex support = SimplexVertex(colliderA.FindFurthestPoint(initialDir, worldTransformA), colliderB.FindFurthestPoint(-initialDir, worldTransformB));
        
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
                
                support = SimplexVertex(colliderA.FindFurthestPoint(direction, worldTransformA), colliderB.FindFurthestPoint(-direction, worldTransformB));
        
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

        void ResolveCollision(Transform& transformA, Transform& transformB, RigidBody& rigidBodyA, RigidBody& rigidBodyB, const Contact& contact)
        {
            if (!contact.HasCollision)
                return;

            float totalMass = rigidBodyA.InvMass + rigidBodyB.InvMass;
            
            if (totalMass == 0)
            {
                return; // two static objects ?		
            }

            if (contact.PenetrationDepth > 0)
            {
                Vector3 correction = contact.Normal * contact.PenetrationDepth;

                if (!rigidBodyA.IsKinematic)
                    transformA.Translate(-correction * (rigidBodyB.IsKinematic ? 1.f : 0.5f));

                if (!rigidBodyB.IsKinematic)
                    transformB.Translate(correction * (rigidBodyA.IsKinematic ? 1.f : 0.5f));
            }

            Vector3 ra = contact.ContactPointA - transformA.Position;
            Vector3 rb = contact.ContactPointB - transformB.Position;

            Vector3 angVelocityA = cross(rigidBodyA.AngularVelocity, ra);
            Vector3 angVelocityB = cross(rigidBodyB.AngularVelocity, rb);
            
            Vector3 fullVelocityA = rigidBodyA.Velocity + angVelocityA;
            Vector3 fullVelocityB = rigidBodyB.Velocity + angVelocityB;
            Vector3 relativeVelocity = fullVelocityB - fullVelocityA;
            
            float contactVelocity = dot(relativeVelocity, contact.Normal);

            if(contactVelocity > 0.0f)
            {
                return;
            }

            auto invTensorA = Matrix3(transformA.Matrix) * rigidBodyA.InvInertiaTensor * transpose(Matrix3(transformA.Matrix));
            auto invTensorB = Matrix3(transformB.Matrix) * rigidBodyB.InvInertiaTensor * transpose(Matrix3(transformB.Matrix));
            
            Vector3 raInertia = invTensorA * cross(ra, contact.Normal);
            Vector3 rbInertia = invTensorB * cross(rb, contact.Normal);

            float invMassSum = rigidBodyA.InvMass + rigidBodyB.InvMass + dot(contact.Normal, cross(raInertia, ra)) + dot(contact.Normal, cross(rbInertia, rb));
            float restitution = 0.4f; // 0.5f for now
            float j = -(1.0f + restitution) * contactVelocity;
            j /= invMassSum;

            if (abs(j) < 0.0001f) return;

            Vector3 impulse = contact.Normal * j;

            if (length(impulse) < 0.001f)
                return;

            if(!rigidBodyA.IsKinematic)
            {
                rigidBodyA.Velocity -= impulse * rigidBodyA.InvMass;
                rigidBodyA.AngularVelocity -= invTensorA * cross(ra, impulse);
            }

            if(!rigidBodyB.IsKinematic)
            {
                rigidBodyB.Velocity += impulse * rigidBodyB.InvMass;
                rigidBodyB.AngularVelocity += invTensorB * cross(rb, impulse);
            }
        }

        void NarrowPhaseCollision(WArray<CollisionPair>& collisionPairs, WArray<Collision>& outCollisions)
        {
            for (auto& contact : ContactCache)
            {
                contact.WasUpdatedThisFrame = false;
            }
            
            for (auto& collisionPair : collisionPairs)
            {
                ColliderComponent& collider1 = Entities[collisionPair.first].get_mut<ColliderComponent>();
                ColliderComponent& collider2 = Entities[collisionPair.second].get_mut<ColliderComponent>();
                Transform& worldTransformA = Entities[collisionPair.first].get_mut<Transform>();
                Transform& worldTransformB = Entities[collisionPair.second].get_mut<Transform>();

            	Contact contact = GJK(collider1, collider2, worldTransformA, worldTransformB);
                // if (contact.HasCollision)
                // {
                //     WD_CORE_INFO("Normal: [{0},{1},{2}], Penetration: {3}", contact.Normal.x, contact.Normal.y, contact.Normal.z, contact.PenetrationDepth);
                //     
                //     collider1->IsColliding = true;
                //     collider2->IsColliding = true;
                //
                //     ResolveCollision(worldTransformA, worldTransformB, rigidBodyA, rigidBodyB, contact);
                // }
                
                if(contact.HasCollision)
                {
                    collider1.IsColliding = collider2.IsColliding = true;
                    outCollisions.Add({collisionPair, contact});
                }
            }

            for (int i = ContactCache.Num() - 1; i >= 0; --i)
            {
                if (!ContactCache[i].WasUpdatedThisFrame)
                {
                    ContactCache.RemoveAt(i);
                }
            }
        }

        void ResolveCollisions(WArray<Collision>& collisions)
        {
            for (auto& collision : collisions)
            {
                auto& entityA = Entities[collision.Colliders.first];
                auto& entityB = Entities[collision.Colliders.second];

                if(!entityA.has<Transform>() || !entityA.has<RigidBody>() || !entityB.has<Transform>() || !entityB.has<RigidBody>())
                {
                    continue;
                }
                
                Transform& worldTransformA = entityA.get_mut<Transform>();
                Transform& worldTransformB = entityB.get_mut<Transform>();
                RigidBody& rigidBodyA = entityA.get_mut<RigidBody>();
                RigidBody& rigidBodyB = entityB.get_mut<RigidBody>();
                
                if(rigidBodyA.IsKinematic && rigidBodyB.IsKinematic)
                {
                    continue;
                }

                if(collision.ContactData.PenetrationDepth < 0.01f)
                {
                    continue;
                }
                
                int idA = collision.Colliders.first;
                int idB = collision.Colliders.second;
                if (idA > idB) std::swap(idA, idB);

                // Try to find existing cached collision.ContactData
                PersistentContact* cached = nullptr;
                for (auto& entry : ContactCache)
                {
                    if (entry.ColliderA == idA && entry.ColliderB == idB)
                    {
                        cached = &entry;
                        break;
                    }
                }

                if (cached)
                {
                    // Blend penetration depth & normal slightly
                    cached->ContactData.Normal = normalize(mix(cached->ContactData.Normal, collision.ContactData.Normal, 0.25f));
                    cached->ContactData.PenetrationDepth = (cached->ContactData.PenetrationDepth + collision.ContactData.PenetrationDepth) * 0.5f;
                    cached->ContactData.ContactPointA = collision.ContactData.ContactPointA;
                    cached->ContactData.ContactPointB = collision.ContactData.ContactPointB;
                    
                    ResolveCollision(worldTransformA, worldTransformB, rigidBodyA, rigidBodyB, cached->ContactData);
                    cached->WasUpdatedThisFrame = true;
                }
                else
                {
                    PersistentContact newEntry;
                    newEntry.ContactData = collision.ContactData;
                    newEntry.ColliderA = idA;
                    newEntry.ColliderB = idB;
                    newEntry.WasUpdatedThisFrame = true;

                    ContactCache.Add(newEntry);

                    ResolveCollision(worldTransformA, worldTransformB, rigidBodyA, rigidBodyB, collision.ContactData);
                }
            }
        }

        void UpdateMeshVertices(CMesh& mesh, ColliderComponent& collider, Transform& transform)
        {
            for (size_t i = 0; i < collider.MeshData.Vertices.Num(); ++i)
            {
                collider.MeshData.Vertices[i] = transform.Matrix * Vector4(mesh.Positions[i], 1.0f);
            }
        }
        
    public:
        CollisionSystem() {}
        
        void Initialize() override
        {
            ECS::World.observer<Transform, ColliderComponent, AABB>().event(flecs::OnAdd).each([&](ECS::Entity e, Transform& transform, ColliderComponent& collider, AABB& bbox)
            {
                Entities.Add(e);
                
                RootNode = BuildBVH(0, Entities.Num());
            });
            
            ECS::World.observer<ColliderComponent>().event(flecs::OnRemove).each([&](ECS::Entity e, ColliderComponent& collider)
            {
                Entities.Remove(e);

                if(Entities.Num() > 0)
                {
                    RootNode = BuildBVH(0, Entities.Num());
                }
            });
            
            ECS::World.system<ColliderComponent>().kind<ECS::OnFixedUpdate>().each([&](ColliderComponent& collider)
            {
                collider.IsColliding = false;
            });
            
            ECS::World.system().kind<ECS::OnFixedUpdate>().each([&]
            {
                WArray<CollisionPair> broadphaseCollisionPairs;
                
                if(RootNode)
                {
                    if (Entities.Num() > 0)
                    {
                        UpdateBVH(RootNode);

                        BroadPhaseCollision(RootNode, RootNode, broadphaseCollisionPairs);
                    }
                }

                WArray<Collision> collisions;
                
                if(!broadphaseCollisionPairs.IsEmpty())
                {
                    NarrowPhaseCollision(broadphaseCollisionPairs, collisions);
                }

                if(!collisions.IsEmpty())
                {
                    ResolveCollisions(collisions);
                }
            });
            
            IsInitialized = true;
        }

        void Deinitialize() override
        {
            if(RootNode)
            {
                delete RootNode;
                RootNode = nullptr;
            }
            IsInitialized = false;
        }
    };
}
