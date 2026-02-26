#pragma once
#include "Waldem/Time.h"
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
        ContactsManifold Manifold;
    };
    
    class WALDEM_API CollisionSystem : public ICoreSystem
    {
        BVHNode* RootNode = nullptr;
        uint MaxIterations = 50;
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

        bool GJK(const ColliderComponent& colliderA, const ColliderComponent& colliderB, Transform& transformA, Transform& transformB, Simplex& outSimplex)
        {
            Vector3 direction(1, 0, 0);

            SimplexVertex support(
                colliderA.FindFurthestPoint(direction, transformA),
                colliderB.FindFurthestPoint(-direction, transformB)
            );
            outSimplex.Add(support);
            direction = -support.Point;

            for (int i = 0; i < MaxIterations; ++i)
            {
                support = SimplexVertex(
                    colliderA.FindFurthestPoint(direction, transformA),
                    colliderB.FindFurthestPoint(-direction, transformB)
                );

                if (dot(support.Point, direction) <= 0)
                    return false;

                outSimplex.Add(support);

                if (GeometryUtils::NextSimplex(outSimplex, direction))
                    return true;
            }

            return false;
        }

        ContactPoint EPA(const Simplex& simplex, const ColliderComponent& colliderA, const ColliderComponent& colliderB, Transform& worldTransformA, Transform& worldTransformB)
        {
            ContactPoint contactPoint;

            WArray<Vector3> polytope;
            WArray<Vector3> sourcePointsA;
            WArray<Vector3> sourcePointsB;

            for (auto vertex : simplex)
            {
                polytope.Add(vertex.Point);
                sourcePointsA.Add(vertex.SupportA);
                sourcePointsB.Add(vertex.SupportB);
            }

            WArray<uint> faces = {
                0, 1, 2,
                0, 3, 1,
                0, 2, 3,
                1, 3, 2
            };

            auto [normals, minFace] = GeometryUtils::GetFaceNormals(polytope, faces);

            Vector3 minNormal = normals[minFace];
            float minDistance = FLT_MAX;

            auto pushSupport = [&](const Vector3& v, const Vector3& a, const Vector3& b)
            {
                polytope.Add(v);
                sourcePointsA.Add(a);
                sourcePointsB.Add(b);
            };

            uint iterations = 0;

            while (minDistance == FLT_MAX && iterations < MaxIterations)
            {
                minNormal = normals[minFace];
                minDistance = normals[minFace].w;

                Vector3 supportA = colliderA.FindFurthestPoint(minNormal, worldTransformA);
                Vector3 supportB = colliderB.FindFurthestPoint(-minNormal, worldTransformB);
                Vector3 support = supportA - supportB;

                float sDistance = dot(minNormal, support);

                if (abs(sDistance - minDistance) > EPA_EPSILON)
                {
                    minDistance = FLT_MAX;
                    std::vector<std::pair<size_t, size_t>> uniqueEdges;

                    for (size_t i = 0; i < normals.Num(); i++)
                    {
                        if (dot((Vector3)normals[i], support) - normals[i].w > 0)
                        {
                            size_t f = i * 3;
                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
                            GeometryUtils::AddIfUniqueEdge(uniqueEdges, faces, f + 2, f);

                            faces[f + 2] = faces.Last(); faces.RemoveLast();
                            faces[f + 1] = faces.Last(); faces.RemoveLast();
                            faces[f] = faces.Last(); faces.RemoveLast();

                            normals[i] = normals.Last();
                            normals.RemoveLast();

                            i--;
                        }
                    }

                    WArray<uint> newFaces;

                    for (auto [edgeIndex1, edgeIndex2] : uniqueEdges)
                    {
                        newFaces.Add(edgeIndex1);
                        newFaces.Add(edgeIndex2);
                        newFaces.Add(polytope.Num());
                    }
                    
                    pushSupport(support, supportA, supportB);

                    auto [newNormals, newMinFace] = GeometryUtils::GetFaceNormals(polytope, newFaces);

                    float oldMinDistance = FLT_MAX;
                    for (size_t i = 0; i < normals.Num(); i++)
                    {
                        if (normals[i].w < oldMinDistance)
                        {
                            oldMinDistance = normals[i].w;
                            minFace = i;
                        }
                    }

                    if (newNormals[newMinFace].w < oldMinDistance)
                    {
                        minFace = newMinFace + normals.Num();
                    }

                    faces.AddRange(newFaces);
                    normals.AddRange(newNormals);
                }

                iterations++;
            }

            // final contact triangle
            size_t idx0 = faces[minFace * 3 + 0];
            size_t idx1 = faces[minFace * 3 + 1];
            size_t idx2 = faces[minFace * 3 + 2];

            Vector3 a = polytope[idx0];
            Vector3 b = polytope[idx1];
            Vector3 c = polytope[idx2];
            
            float distance = dot(a, minNormal);
            Vector3 projectedPoint = -distance * minNormal;

            Vector3 a0 = sourcePointsA[idx0];
            Vector3 a1 = sourcePointsA[idx1];
            Vector3 a2 = sourcePointsA[idx2];

            Vector3 b0 = sourcePointsB[idx0];
            Vector3 b1 = sourcePointsB[idx1];
            Vector3 b2 = sourcePointsB[idx2];

            auto barycentricsResult = GeometryUtils::GetBarycentricCoordinates(projectedPoint, a, b, c);

            auto pointA = barycentricsResult.u * a0 + barycentricsResult.v * a1 + barycentricsResult.w * a2;
            auto pointB = barycentricsResult.u * b0 + barycentricsResult.v * b1 + barycentricsResult.w * b2;

            contactPoint.PositionA = pointA;
            contactPoint.PositionB = pointB;
            contactPoint.Position = (pointA + pointB) * 0.5f;
            contactPoint.Normal = normalize(minNormal);
            contactPoint.Penetration = minDistance + 0.001f;
            contactPoint.CalculateBasic();

            return contactPoint;
        }

        ContactsManifold BuildContactManifold(const EPAResult& epa)
        {
            ContactsManifold manifold;

            ContactPoint contactPoint;
            contactPoint.PositionA = epa.ContactPositionA;
            contactPoint.PositionB = epa.ContactPositionB;
            contactPoint.Position = epa.ContactCenterPosition;
            contactPoint.Penetration = epa.PenetrationDepth;
            contactPoint.Normal = epa.Normal;
            contactPoint.CalculateBasic();

            manifold.Points.Add(contactPoint);
            return manifold;
        }

        void NarrowPhaseCollision(WArray<CollisionPair>& collisionPairs, WArray<Collision>& outCollisions)
        {
            for (auto& collisionPair : collisionPairs)
            {
                auto& entity1 = Entities[collisionPair.first];
                auto& entity2 = Entities[collisionPair.second];
                ColliderComponent& colliderA = entity1.get_mut<ColliderComponent>();
                ColliderComponent& colliderB = entity2.get_mut<ColliderComponent>();
                Transform& worldTransformA = entity1.get_mut<Transform>();
                Transform& worldTransformB = entity2.get_mut<Transform>();
                
                Simplex simplex;
            	if(GJK(colliderA, colliderB, worldTransformA, worldTransformB, simplex))
            	{
            	    auto contactPoint = EPA(simplex, colliderA, colliderB, worldTransformA, worldTransformB);
            	    
            	    if(contactPoint.Penetration > 0 && length(contactPoint.Normal) > 0.9f)
                    {
            	        //TODO: Multiple contact points
                        ContactsManifold manifold;
            	        manifold.Points.Add(contactPoint);
            	        outCollisions.Add({collisionPair, manifold});
                    }
            	}
            }
        }

        void ResolveCollisions(WArray<Collision>& collisions)
        {
            for (auto& collision : collisions)
            {
                int idA = collision.Colliders.first;
                int idB = collision.Colliders.second;
                if (idA > idB) std::swap(idA, idB);
                
                auto& entityA = Entities[idA];
                auto& entityB = Entities[idB];

                if(!entityA.has<Transform>() || !entityA.has<RigidBody>() || !entityB.has<Transform>() || !entityB.has<RigidBody>())
                {
                    continue;
                }
                
                Transform& worldTransformA = entityA.get_mut<Transform>();
                Transform& worldTransformB = entityB.get_mut<Transform>();
                RigidBody& rigidBodyA = entityA.get_mut<RigidBody>();
                RigidBody& rigidBodyB = entityB.get_mut<RigidBody>();

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

                CollisionEventType eventType = CollisionEventType::Enter;

                if (cached)
                {
                    eventType = CollisionEventType::Stay;
                    cached->Manifold = collision.Manifold;
                    cached->WasUpdatedThisFrame = true;
                    cached->FramesAlive++;
                }
                else
                {
                    PersistentContact newEntry;
                    newEntry.Manifold = collision.Manifold;
                    newEntry.ColliderA = idA;
                    newEntry.ColliderB = idB;
                    newEntry.WasUpdatedThisFrame = true;

                    ContactCache.Add(newEntry);
                }

                TriggerCollisionEvent(entityA, entityB, collision.Manifold, eventType);

                if(!rigidBodyA.IsKinematic || !rigidBodyB.IsKinematic)
                {
                    if (rigidBodyA.IsSleeping && rigidBodyB.IsSleeping)
                        continue;

                    ResolveCollisionSimplified(worldTransformA, worldTransformB, rigidBodyA, rigidBodyB, collision.Manifold);
                }
            }
        }

        void ResolveCollisionSimplified(Transform& transformA, Transform& transformB, RigidBody& rigidBodyA, RigidBody& rigidBodyB, const ContactsManifold& manifold)
        {
            if (manifold.Points.IsEmpty())
                return;

            auto contactPoint = manifold.Points[0]; // Using only the first contact point for simplified resolution

            auto massA = rigidBodyA.IsKinematic ? 0.f : rigidBodyA.GetInvMass();
            auto massB = rigidBodyB.IsKinematic ? 0.f : rigidBodyB.GetInvMass();
            float totalMass = massA + massB;
            if (totalMass == 0)
                return;

            float perPointScale = 1.0f / (float)manifold.Points.Num();

            Vector3 normal = contactPoint.Normal;
            
            Vector3 velA = rigidBodyA.Velocity;
            Vector3 velB = rigidBodyB.Velocity;
            
            Vector3 relativeVelocity = velB - velA;
            float contactVelocity = dot(relativeVelocity, normal);
            
            bool hasPenetration = contactPoint.Penetration > 0.0001f;
            
            if (!hasPenetration)
                return;
            
            float bounciness = (rigidBodyA.Bounciness + rigidBodyB.Bounciness) * 0.5f;
            
            // disable restitution for tiny velocities
            float restitutionThreshold = 2.0f;
            float restitution = fabs(contactVelocity) > restitutionThreshold ? bounciness : 0.0f;
            
            // skip near-resting tiny velocities
            if (fabs(contactVelocity) < 0.01f && contactPoint.Penetration < 0.002f)
                return;
            
            float j = -(1.0f + restitution) * contactVelocity;
            j *= perPointScale;
            
            if (fabs(j) < 1e-5f)
                return;
            
            Vector3 impulse = normal * j;
            
            // Apply normal impulse
            if (!rigidBodyA.IsKinematic && !rigidBodyA.IsSleeping)
            {
                rigidBodyA.Velocity -= impulse * massA;
            }
            if (!rigidBodyB.IsKinematic && !rigidBodyB.IsSleeping)
            {
                rigidBodyB.Velocity += impulse * massB;
            }

            if (contactPoint.Penetration > 0)
            {
                Vector3 correction = normal * contactPoint.Penetration * perPointScale;

                if (massA > 0)
                    transformA.Position -= correction * (massA / totalMass);

                if (massB > 0)
                    transformB.Position += correction * (massB / totalMass);
            }

            if(!rigidBodyA.IsKinematic)
            {
                float groundThreshold = cos(glm::radians(rigidBodyA.MaxSlope));
                if (groundThreshold < 0) groundThreshold = 0;

                rigidBodyA.IsGrounded = dot(normal, Vector3(0,1,0)) > groundThreshold;

                if (rigidBodyA.IsGrounded)
                {
                    float vDot = dot(rigidBodyA.Velocity, normal);
                    rigidBodyA.Velocity = normal * vDot;

                    if (length(rigidBodyA.Velocity) < 1e-4f)
                    {
                        rigidBodyA.Velocity = Vector3(0);
                    }
                }
            }

            if(!rigidBodyB.IsKinematic)
            {
                float groundThreshold = cos(glm::radians(rigidBodyB.MaxSlope)) - 0.05f;
                if (groundThreshold < 0) groundThreshold = 0;

                rigidBodyB.IsGrounded = dot(normal, Vector3(0,1,0)) > groundThreshold;

                if (rigidBodyB.IsGrounded)
                {
                    float vDot = dot(rigidBodyB.Velocity, normal);
                    rigidBodyB.Velocity = normal * vDot;

                    if (length(rigidBodyB.Velocity) < 1e-4f)
                    {
                        rigidBodyB.Velocity = Vector3(0);
                    }
                }
            }
        }

        void ResolveCollisionOld(Transform& transformA, Transform& transformB, RigidBody& rigidBodyA, RigidBody& rigidBodyB, const ContactsManifold& manifold)
        {
            if (manifold.Points.IsEmpty())
                return;

            auto massA = rigidBodyA.IsKinematic ? 0.f : rigidBodyA.GetInvMass();
            auto massB = rigidBodyB.IsKinematic ? 0.f : rigidBodyB.GetInvMass();
            float totalMass = massA + massB;
            if (totalMass == 0)
                return;

            float perPointScale = 1.0f / (float)manifold.Points.Num();

            for (size_t i = 0; i < manifold.Points.Num(); ++i)
            {
                const ContactPoint& contactPoint = manifold.Points[i];
                Vector3 normal = contactPoint.Normal;
                
                Vector3 contactPosition = contactPoint.Position;
                
                Vector3 ra = contactPosition - transformA.Position;
                Vector3 rb = contactPosition - transformB.Position;
                
                // Vector3 velA = rigidBodyA.Velocity + cross(rigidBodyA.AngularVelocity, ra);
                // Vector3 velB = rigidBodyB.Velocity + cross(rigidBodyB.AngularVelocity, rb);
                Vector3 velA = rigidBodyA.Velocity;
                Vector3 velB = rigidBodyB.Velocity;
                
                Vector3 relativeVelocity = velB - velA;
                float contactVelocity = dot(relativeVelocity, normal);
                
                bool hasPenetration = contactPoint.Penetration > 0.0001f;
                
                if (!hasPenetration)
                    continue;
                
                // Matrix3 R = Matrix3(transformA.RotationQuat);
                // Matrix3 inTensorA = R * rigidBodyA.InertiaTensor * transpose(R);
                // R = Matrix3(transformB.RotationQuat);
                // Matrix3 inTensorB = R * rigidBodyB.InertiaTensor * transpose(R);
                
                // float invMassSum = totalMass;
                // if (!rigidBodyA.IsKinematic)
                // {
                //     Vector3 rxn = cross(ra, normal);
                //     Vector3 raInertia = inTensorA * rxn;
                //     invMassSum += dot(rxn, raInertia);
                // }
                // if (!rigidBodyB.IsKinematic)
                // {
                //     Vector3 rxn = cross(rb, normal);
                //     Vector3 rbInertia = inTensorB * rxn;
                //     invMassSum += dot(rxn, rbInertia);
                // }
                //
                // if (invMassSum == 0.0f)
                //     continue;
                
                float bounciness = (rigidBodyA.Bounciness + rigidBodyB.Bounciness) * 0.5f;
                
                // disable restitution for tiny velocities
                float restitutionThreshold = 2.0f;
                float restitution = fabs(contactVelocity) > restitutionThreshold ? bounciness : 0.0f;
                
                // skip near-resting tiny velocities
                if (fabs(contactVelocity) < 0.01f && contactPoint.Penetration < 0.002f)
                    continue;
                
                float j = -(1.0f + restitution) * contactVelocity;
                // j /= invMassSum;
                j *= perPointScale;
                
                if (fabs(j) < 1e-5f)
                    continue;
                
                Vector3 impulse = normal * j;
                
                // Apply normal impulse
                if (!rigidBodyA.IsKinematic && !rigidBodyA.IsSleeping)
                {
                    rigidBodyA.Velocity -= impulse * massA;
                    // rigidBodyA.AngularVelocity -= inTensorA * cross(ra, impulse);
                }
                if (!rigidBodyB.IsKinematic && !rigidBodyB.IsSleeping)
                {
                    rigidBodyB.Velocity += impulse * massB;
                    // rigidBodyB.AngularVelocity += inTensorB * cross(rb, impulse);
                }
                
                // // --- Friction ---
                // Vector3 raVel = cross(rigidBodyA.AngularVelocity, ra);
                // Vector3 rbVel = cross(rigidBodyB.AngularVelocity, rb);
                // Vector3 relVel = (rigidBodyB.Velocity + rbVel) - (rigidBodyA.Velocity + raVel);
                //
                // Vector3 tangent = relVel - normal * dot(relVel, normal);
                // float tangentLen = length(tangent);
                // if (tangentLen > 1e-6f)
                //     tangent /= tangentLen;
                // else
                //     tangent = Vector3(0);
                //
                // float tangentMass = totalMass;
                // if (!rigidBodyA.IsKinematic)
                // {
                //     Vector3 rxt = cross(ra, tangent);
                //     tangentMass += dot(rxt, inTensorA * rxt);
                // }
                // if (!rigidBodyB.IsKinematic)
                // {
                //     Vector3 rxt = cross(rb, tangent);
                //     tangentMass += dot(rxt, inTensorB * rxt);
                // }
                //
                // if (tangentMass > 0.0f)
                // {
                //     float jt = -dot(relVel, tangent) / tangentMass;
                //
                //     float mu = (rigidBodyA.Friction + rigidBodyB.Friction) * 0.5f;
                //     float maxFriction = fabs(j) * mu;
                //     jt = glm::clamp(jt, -maxFriction, maxFriction);
                //
                //     Vector3 frictionImpulse = tangent * jt;
                //     frictionImpulse *= perPointScale;
                //
                //     if (!rigidBodyA.IsKinematic)
                //     {
                //         rigidBodyA.Velocity -= frictionImpulse * massA;
                //         rigidBodyA.AngularVelocity -= inTensorA * cross(ra, frictionImpulse);
                //     }
                //     if (!rigidBodyB.IsKinematic)
                //     {
                //         rigidBodyB.Velocity += frictionImpulse * massB;
                //         rigidBodyB.AngularVelocity += inTensorB * cross(rb, frictionImpulse);
                //     }
                // }

                if (contactPoint.Penetration > 0)
                {
                    Vector3 correction = normal * contactPoint.Penetration * perPointScale;

                    if (massA > 0)
                        transformA.Position -= correction * (massA / totalMass);

                    if (massB > 0)
                        transformB.Position += correction * (massB / totalMass);
                }
            }
        }

        void TriggerCollisionEvent(ECS::Entity& entityA, ECS::Entity& entityB, const ContactsManifold& manifold, CollisionEventType type)
        {
            auto& colliderA = entityA.get_mut<ColliderComponent>();
            auto& colliderB = entityB.get_mut<ColliderComponent>();

            switch (type)
            {
            case CollisionEventType::Enter:
                if (colliderA.OnCollisionEnter) colliderA.OnCollisionEnter(entityB, manifold);
                if (colliderB.OnCollisionEnter) colliderB.OnCollisionEnter(entityA, manifold);
                break;

            case CollisionEventType::Stay:
                if (colliderA.OnCollisionStay) colliderA.OnCollisionStay(entityB, manifold);
                if (colliderB.OnCollisionStay) colliderB.OnCollisionStay(entityA, manifold);
                break;

            case CollisionEventType::Exit:
                if (colliderA.OnCollisionExit) colliderA.OnCollisionExit(entityB, manifold);
                if (colliderB.OnCollisionExit) colliderB.OnCollisionExit(entityA, manifold);
                break;
            }
        }

        void HandleCachedContacts()
        {
            // Handle exit events
            for (int i = ContactCache.Num() - 1; i >= 0; --i)
            {
                auto& contact = ContactCache[i];
                if (!contact.WasUpdatedThisFrame)
                {
                    auto& entityA = Entities[contact.ColliderA];
                    auto& entityB = Entities[contact.ColliderB];
                    TriggerCollisionEvent(entityA, entityB, contact.Manifold, CollisionEventType::Exit);
                    ContactCache.RemoveAt(i);
                }
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
            
            // ECS::World.system().kind(flecs::OnUpdate).each([&]
            ECS::World.system().kind<ECS::OnFixedUpdate>().each([&]
            {
                for (auto& contact : ContactCache)
                {
                    contact.WasUpdatedThisFrame = false;
                }
                
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

                HandleCachedContacts();
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
