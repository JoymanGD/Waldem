#include "wdpch.h"
#include "PhysXSystem.h"

#if WD_WITH_PHYSX

#include "Waldem/Editor/EditorSimulation.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Time.h"
#include "PxPhysicsAPI.h"

namespace Waldem
{
    namespace
    {
        using namespace physx;

        class PhysXErrorCallback final : public PxErrorCallback
        {
        public:
            void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
            {
                WD_CORE_ERROR("PhysX error {} at {}:{} - {}", static_cast<int>(code), file != nullptr ? file : "", line, message != nullptr ? message : "");
            }
        };

        struct PhysXState
        {
            PxDefaultAllocator Allocator;
            PhysXErrorCallback ErrorCallback;
            PxFoundation* Foundation = nullptr;
            PxPhysics* Physics = nullptr;
            PxDefaultCpuDispatcher* Dispatcher = nullptr;
            PxScene* Scene = nullptr;
            PxMaterial* DefaultMaterial = nullptr;
            std::unordered_map<uint64, PxRigidActor*> Actors;
            std::unordered_map<uint64, Vector3> PreviousPositions;
            std::unordered_set<uint64> DirtyEntities;
            bool IsSynchronizing = false;
            Vector3 Gravity = Vector3(0.0f, -9.81f, 0.0f);
        };

        PhysXState& GetState()
        {
            static PhysXState state;
            return state;
        }

        PxVec3 ToPx(const Vector3& v)
        {
            return PxVec3(v.x, v.y, v.z);
        }

        Vector3 FromPx(const PxVec3& v)
        {
            return Vector3(v.x, v.y, v.z);
        }

        PxQuat ToPx(const Quaternion& q)
        {
            return PxQuat(q.x, q.y, q.z, q.w);
        }

        Quaternion FromPx(const PxQuat& q)
        {
            return Quaternion(q.w, q.x, q.y, q.z);
        }

        PxTransform ToPxTransform(const Transform& transform)
        {
            return PxTransform(ToPx(transform.Position), ToPx(transform.RotationQuat));
        }

        Vector3 GetColliderScale(const Transform& transform)
        {
            return glm::abs(transform.LocalScale);
        }

        bool ComputeRotateTowardsRotation(const Transform& transform, const Vector3& target, float maxDegreesDelta, Quaternion& outRotation)
        {
            Vector3 direction = target - transform.Position;
            const float directionLength = length(direction);
            if(directionLength <= 1e-6f)
            {
                outRotation = transform.RotationQuat;
                return false;
            }

            direction /= directionLength;
            const Vector3 adjustedUp = glm::abs(dot(direction, Vector3(0, 1, 0))) > 0.99f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);

            const Quaternion currentRotation = normalize(transform.RotationQuat);
            const Quaternion targetRotation = normalize(quatLookAt(direction, adjustedUp));

            const float maxRadiansDelta = std::max(0.0f, glm::radians(maxDegreesDelta));
            const float cosHalfAngle = glm::clamp(glm::abs(dot(currentRotation, targetRotation)), 0.0f, 1.0f);
            const float angle = 2.0f * acos(cosHalfAngle);

            if(angle <= 1e-6f || angle <= maxRadiansDelta)
            {
                outRotation = targetRotation;
                return true;
            }

            if(maxRadiansDelta <= 1e-6f)
            {
                outRotation = currentRotation;
                return false;
            }

            const float t = glm::clamp(maxRadiansDelta / angle, 0.0f, 1.0f);
            outRotation = normalize(slerp(currentRotation, targetRotation, t));
            return true;
        }

        PxVec3 RotateVector(const Quaternion& rotation, const Vector3& value)
        {
            return ToPx(rotation * value);
        }

        uint64 GetEntityId(const PxActor* actor)
        {
            return actor == nullptr ? 0 : static_cast<uint64>(reinterpret_cast<uintptr_t>(actor->userData));
        }

        ECS::Entity GetEntityById(uint64 entityId)
        {
            return entityId == 0 ? ECS::Entity{} : ECS::World.entity(static_cast<ECS::EntityT>(entityId));
        }

        ContactsManifold BuildManifold(const PxContactPair& pair)
        {
            ContactsManifold manifold;
            if(pair.contactCount == 0)
            {
                return manifold;
            }

            std::vector<PxContactPairPoint> points(pair.contactCount);
            const PxU32 count = pair.extractContacts(points.data(), static_cast<PxU32>(points.size()));
            for(PxU32 i = 0; i < count; ++i)
            {
                ContactPoint contactPoint;
                contactPoint.Position = FromPx(points[i].position);
                contactPoint.PositionA = contactPoint.Position;
                contactPoint.PositionB = contactPoint.Position;
                contactPoint.Normal = normalize(FromPx(points[i].normal));
                contactPoint.Penetration = 0.0f;
                contactPoint.CalculateBasic();
                manifold.Points.Add(contactPoint);
            }

            return manifold;
        }

        void DispatchCollisionEvent(uint64 entityAId, uint64 entityBId, const ContactsManifold& manifold, CollisionEventType type)
        {
            ECS::Entity entityA = GetEntityById(entityAId);
            ECS::Entity entityB = GetEntityById(entityBId);
            if(!entityA.is_alive() || !entityB.is_alive() || !entityA.has<ColliderComponent>() || !entityB.has<ColliderComponent>())
            {
                return;
            }

            auto& colliderA = entityA.get_mut<ColliderComponent>();
            auto& colliderB = entityB.get_mut<ColliderComponent>();

            if(type != CollisionEventType::Exit && !manifold.Points.IsEmpty())
            {
                const Vector3 up(0.0f, 1.0f, 0.0f);
                const ContactPoint& contactPoint = manifold.Points[0];
                const Vector3 normal = contactPoint.Normal;

                if(entityA.has<RigidBody>() && entityA.has<Transform>())
                {
                    auto& rigidBodyA = entityA.get_mut<RigidBody>();
                    const auto& transformA = entityA.get<Transform>();
                    const float groundThreshold = std::max(0.0f, cos(glm::radians(rigidBodyA.MaxSlope)));
                    const Vector3 toContactA = contactPoint.Position - transformA.Position;
                    const Vector3 supportNormalA = dot(normal, toContactA) <= 0.0f ? normal : -normal;
                    rigidBodyA.IsGrounded = rigidBodyA.IsGrounded || dot(supportNormalA, up) > groundThreshold;
                }

                if(entityB.has<RigidBody>() && entityB.has<Transform>())
                {
                    auto& rigidBodyB = entityB.get_mut<RigidBody>();
                    const auto& transformB = entityB.get<Transform>();
                    const float groundThreshold = std::max(0.0f, cos(glm::radians(rigidBodyB.MaxSlope)));
                    const Vector3 toContactB = contactPoint.Position - transformB.Position;
                    const Vector3 supportNormalB = dot(normal, toContactB) <= 0.0f ? normal : -normal;
                    rigidBodyB.IsGrounded = rigidBodyB.IsGrounded || dot(supportNormalB, up) > groundThreshold;
                }
            }

            if(colliderA.IsTrigger)
            {
                switch(type)
                {
                case CollisionEventType::Enter:
                    if(colliderA.OnTriggerEnter) colliderA.OnTriggerEnter(entityB);
                    break;
                case CollisionEventType::Stay:
                    if(colliderA.OnTriggerStay) colliderA.OnTriggerStay(entityB);
                    break;
                case CollisionEventType::Exit:
                    if(colliderA.OnTriggerExit) colliderA.OnTriggerExit(entityB);
                    break;
                }
            }
            else
            {
                switch(type)
                {
                case CollisionEventType::Enter:
                    if(colliderA.OnCollisionEnter) colliderA.OnCollisionEnter(entityB, manifold);
                    break;
                case CollisionEventType::Stay:
                    if(colliderA.OnCollisionStay) colliderA.OnCollisionStay(entityB, manifold);
                    break;
                case CollisionEventType::Exit:
                    if(colliderA.OnCollisionExit) colliderA.OnCollisionExit(entityB, manifold);
                    break;
                }
            }

            if(colliderB.IsTrigger)
            {
                switch(type)
                {
                case CollisionEventType::Enter:
                    if(colliderB.OnTriggerEnter) colliderB.OnTriggerEnter(entityB);
                    break;
                case CollisionEventType::Stay:
                    if(colliderB.OnTriggerStay) colliderB.OnTriggerStay(entityB);
                    break;
                case CollisionEventType::Exit:
                    if(colliderB.OnTriggerExit) colliderB.OnTriggerExit(entityB);
                    break;
                }
            }
            else
            {
                switch(type)
                {
                case CollisionEventType::Enter:
                    if(colliderB.OnCollisionEnter) colliderB.OnCollisionEnter(entityB, manifold);
                    break;
                case CollisionEventType::Stay:
                    if(colliderB.OnCollisionStay) colliderB.OnCollisionStay(entityB, manifold);
                    break;
                case CollisionEventType::Exit:
                    if(colliderB.OnCollisionExit) colliderB.OnCollisionExit(entityB, manifold);
                    break;
                }
            }
        }

        class PhysXSimulationCallback final : public PxSimulationEventCallback
        {
        public:
            void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
            void onWake(PxActor**, PxU32) override {}
            void onSleep(PxActor**, PxU32) override {}
            void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}

            void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override
            {
                const uint64 entityAId = GetEntityId(pairHeader.actors[0]);
                const uint64 entityBId = GetEntityId(pairHeader.actors[1]);
                if(entityAId == 0 || entityBId == 0)
                {
                    return;
                }

                for(PxU32 i = 0; i < nbPairs; ++i)
                {
                    const PxContactPair& pair = pairs[i];
                    const ContactsManifold manifold = BuildManifold(pair);

                    if(pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
                    {
                        DispatchCollisionEvent(entityAId, entityBId, manifold, CollisionEventType::Enter);
                    }
                    if(pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
                    {
                        DispatchCollisionEvent(entityAId, entityBId, manifold, CollisionEventType::Stay);
                    }
                    if(pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
                    {
                        DispatchCollisionEvent(entityAId, entityBId, manifold, CollisionEventType::Exit);
                    }
                }
            }

            void onTrigger(PxTriggerPair* pairs, PxU32 count) override
            {
                for(PxU32 i = 0; i < count; ++i)
                {
                    const PxTriggerPair& pair = pairs[i];
                    if(pair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
                    {
                        continue;
                    }

                    const uint64 triggerEntityId = GetEntityId(pair.triggerActor);
                    const uint64 otherEntityId = GetEntityId(pair.otherActor);
                    if(triggerEntityId == 0 || otherEntityId == 0)
                    {
                        continue;
                    }

                    ContactsManifold manifold;
                    if(pair.status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
                    {
                        DispatchCollisionEvent(triggerEntityId, otherEntityId, manifold, CollisionEventType::Enter);
                    }
                    else if(pair.status & PxPairFlag::eNOTIFY_TOUCH_LOST)
                    {
                        DispatchCollisionEvent(triggerEntityId, otherEntityId, manifold, CollisionEventType::Exit);
                    }
                }
            }
        };

        PhysXSimulationCallback& GetSimulationCallback()
        {
            static PhysXSimulationCallback callback;
            return callback;
        }

        PxFilterFlags FilterShader(
            PxFilterObjectAttributes attributes0,
            PxFilterData,
            PxFilterObjectAttributes attributes1,
            PxFilterData,
            PxPairFlags& pairFlags,
            const void*,
            PxU32)
        {
            if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
            {
                pairFlags = PxPairFlag::eTRIGGER_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST;
                return PxFilterFlag::eDEFAULT;
            }

            pairFlags = PxPairFlag::eCONTACT_DEFAULT
                | PxPairFlag::eNOTIFY_TOUCH_FOUND
                | PxPairFlag::eNOTIFY_TOUCH_PERSISTS
                | PxPairFlag::eNOTIFY_TOUCH_LOST
                | PxPairFlag::eNOTIFY_CONTACT_POINTS;
            return PxFilterFlag::eDEFAULT;
        }

        void ReleaseActor(uint64 entityId)
        {
            auto& state = GetState();
            auto it = state.Actors.find(entityId);
            if(it == state.Actors.end())
            {
                return;
            }

            if(state.Scene != nullptr && it->second != nullptr)
            {
                state.Scene->removeActor(*it->second);
                it->second->release();
            }

            state.Actors.erase(it);
        }

        PxShape* CreateShape(const ColliderComponent& collider, const Transform& transform, PxPhysics& physics, PxMaterial& material)
        {
            PxShape* shape = nullptr;
            const Vector3 colliderScale = GetColliderScale(transform);

            switch(collider.Type)
            {
            case Sphere:
            {
                const float radiusScale = std::max(colliderScale.x, std::max(colliderScale.y, colliderScale.z));
                shape = physics.createShape(PxSphereGeometry(collider.SphereRadius * radiusScale), material, true);
                break;
            }
            case Box:
            {
                const Vector3 halfExtents = (collider.BoxSize * colliderScale) * 0.5f;
                shape = physics.createShape(PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z), material, true);
                if(shape != nullptr)
                {
                    shape->setLocalPose(PxTransform(ToPx(collider.BoxOffset * colliderScale)));
                }
                break;
            }
            case Capsule:
            {
                const float radiusScale = std::max(colliderScale.x, colliderScale.z);
                const float radius = collider.SphereRadius * radiusScale;
                const float halfHeight = collider.CapsuleHeight * 0.5f * colliderScale.y;
                shape = physics.createShape(PxCapsuleGeometry(radius, halfHeight), material, true);
                if(shape != nullptr)
                {
                    shape->setLocalPose(PxTransform(PxVec3(0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f))));
                }
                break;
            }
            case Mesh:
                WD_CORE_WARN("PhysX mesh colliders are not implemented yet; skipping collider creation for a mesh collider.");
                break;
            default:
                break;
            }

            if(shape != nullptr)
            {
                shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
                if(collider.IsTrigger)
                {
                    shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
                    shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
                }
            }

            return shape;
        }

        void ApplyAxisConstraints(const RigidBody& rigidBody, PxRigidDynamic& dynamicActor)
        {
            dynamicActor.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, rigidBody.FreezePositionX);
            dynamicActor.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, rigidBody.FreezePositionY);
            dynamicActor.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, rigidBody.FreezePositionZ);
            dynamicActor.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, rigidBody.FreezeRotationX);
            dynamicActor.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, rigidBody.FreezeRotationY);
            dynamicActor.setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, rigidBody.FreezeRotationZ);
        }

        bool QueryGrounded(ECS::Entity entity)
        {
            auto& state = GetState();
            if(state.Scene == nullptr || !entity.is_alive() || !entity.has<RigidBody>() || !entity.has<Transform>() || !entity.has<ColliderComponent>())
            {
                return false;
            }

            const auto& rigidBody = entity.get<RigidBody>();
            const auto& transform = entity.get<Transform>();
            const auto& collider = entity.get<ColliderComponent>();
            const Vector3 colliderScale = GetColliderScale(transform);

            const float probeDistance = 0.12f;
            const float groundThreshold = std::max(0.0f, cos(glm::radians(rigidBody.MaxSlope)));
            const PxVec3 down(0.0f, -1.0f, 0.0f);

            PxVec3 origin = ToPx(transform.Position);
            float maxDistance = probeDistance;

            switch(collider.Type)
            {
            case Sphere:
            {
                const float radiusScale = std::max(colliderScale.x, std::max(colliderScale.y, colliderScale.z));
                maxDistance += collider.SphereRadius * radiusScale;
                break;
            }
            case Box:
            {
                const Vector3 localCenter = collider.BoxOffset * colliderScale;
                origin = ToPx(transform.Position) + RotateVector(transform.RotationQuat, localCenter);
                maxDistance += collider.BoxSize.y * colliderScale.y * 0.5f;
                break;
            }
            case Capsule:
            {
                const float radiusScale = std::max(colliderScale.x, colliderScale.z);
                maxDistance += collider.CapsuleHeight * 0.5f * colliderScale.y + collider.SphereRadius * radiusScale;
                break;
            }
            default:
                return false;
            }

            struct QueryFilter final : PxQueryFilterCallback
            {
                const PxActor* IgnoredActor = nullptr;

                PxQueryHitType::Enum preFilter(const PxFilterData&, const PxShape* shape, const PxRigidActor* actor, PxHitFlags&) override
                {
                    if(actor == IgnoredActor || shape == nullptr || shape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE))
                    {
                        return PxQueryHitType::eNONE;
                    }

                    return PxQueryHitType::eBLOCK;
                }

                PxQueryHitType::Enum postFilter(const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override
                {
                    return PxQueryHitType::eBLOCK;
                }
            } filterCallback;

            auto actorIt = state.Actors.find(static_cast<uint64>(entity.id()));
            if(actorIt != state.Actors.end())
            {
                filterCallback.IgnoredActor = actorIt->second;
            }

            PxQueryFilterData filterData(PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER);
            PxRaycastBuffer hit;
            if(!state.Scene->raycast(origin, down, maxDistance, hit, PxHitFlag::eDEFAULT, filterData, &filterCallback))
            {
                return false;
            }

            return dot(FromPx(hit.block.normal), Vector3(0.0f, 1.0f, 0.0f)) > groundThreshold;
        }

        bool QuerySteepObstacleAhead(ECS::Entity entity, const Vector3& direction, float probeDistance, Vector3& outNormal)
        {
            auto& state = GetState();
            if(state.Scene == nullptr || !entity.is_alive() || !entity.has<RigidBody>() || !entity.has<Transform>() || !entity.has<ColliderComponent>())
            {
                return false;
            }

            Vector3 horizontalDirection(direction.x, 0.0f, direction.z);
            const float horizontalLength = length(horizontalDirection);
            if(horizontalLength <= 1e-4f)
            {
                return false;
            }

            const auto& rigidBody = entity.get<RigidBody>();
            const auto& transform = entity.get<Transform>();
            const auto& collider = entity.get<ColliderComponent>();
            const Vector3 colliderScale = GetColliderScale(transform);
            const float groundThreshold = std::max(0.0f, cos(glm::radians(rigidBody.MaxSlope)));

            horizontalDirection /= horizontalLength;
            PxVec3 origin = ToPx(transform.Position);
            probeDistance = std::max(0.15f, probeDistance);

            switch(collider.Type)
            {
            case Sphere:
            {
                const float radiusScale = std::max(colliderScale.x, std::max(colliderScale.y, colliderScale.z));
                origin.y -= collider.SphereRadius * radiusScale * 0.5f;
                break;
            }
            case Box:
            {
                const Vector3 localCenter = collider.BoxOffset * colliderScale;
                origin = ToPx(transform.Position) + RotateVector(transform.RotationQuat, localCenter);
                origin.y -= collider.BoxSize.y * colliderScale.y * 0.5f - std::min(0.05f, collider.BoxSize.y * colliderScale.y * 0.25f);
                break;
            }
            case Capsule:
            {
                const float radiusScale = std::max(colliderScale.x, colliderScale.z);
                const float radius = collider.SphereRadius * radiusScale;
                const float halfHeight = collider.CapsuleHeight * 0.5f * colliderScale.y;
                origin.y -= std::max(0.0f, halfHeight + radius - radius * 0.75f);
                break;
            }
            default:
                return false;
            }

            struct QueryFilter final : PxQueryFilterCallback
            {
                const PxActor* IgnoredActor = nullptr;

                PxQueryHitType::Enum preFilter(const PxFilterData&, const PxShape* shape, const PxRigidActor* actor, PxHitFlags&) override
                {
                    if(actor == IgnoredActor || shape == nullptr || shape->getFlags().isSet(PxShapeFlag::eTRIGGER_SHAPE))
                    {
                        return PxQueryHitType::eNONE;
                    }

                    return PxQueryHitType::eBLOCK;
                }

                PxQueryHitType::Enum postFilter(const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override
                {
                    return PxQueryHitType::eBLOCK;
                }
            } filterCallback;

            auto actorIt = state.Actors.find(static_cast<uint64>(entity.id()));
            if(actorIt != state.Actors.end())
            {
                filterCallback.IgnoredActor = actorIt->second;
            }

            PxQueryFilterData filterData(PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER);
            PxRaycastBuffer hit;
            if(!state.Scene->raycast(origin, ToPx(horizontalDirection), probeDistance, hit, PxHitFlag::eDEFAULT, filterData, &filterCallback))
            {
                return false;
            }

            outNormal = FromPx(hit.block.normal);
            return dot(outNormal, Vector3(0.0f, 1.0f, 0.0f)) <= groundThreshold;
        }

        void PreventSteepSlopeAcceleration(ECS::Entity entity)
        {
            if(!entity.is_alive() || !entity.has<RigidBody>())
            {
                return;
            }

            auto& rigidBody = entity.get_mut<RigidBody>();
            if(rigidBody.IsKinematic)
            {
                return;
            }

            Vector3 horizontalVelocity(rigidBody.Velocity.x, 0.0f, rigidBody.Velocity.z);
            const float horizontalSpeed = length(horizontalVelocity);
            if(horizontalSpeed <= 1e-4f)
            {
                return;
            }

            Vector3 obstacleNormal(0.0f);
            const float probeDistance = std::max(0.15f, horizontalSpeed * Time::FixedDeltaTime + 0.08f);
            if(!QuerySteepObstacleAhead(entity, horizontalVelocity, probeDistance, obstacleNormal))
            {
                return;
            }

            Vector3 wallNormal(obstacleNormal.x, 0.0f, obstacleNormal.z);
            const float wallNormalLength = length(wallNormal);
            if(wallNormalLength <= 1e-4f)
            {
                return;
            }

            wallNormal /= wallNormalLength;

            const float velocityIntoWall = dot(horizontalVelocity, wallNormal);
            if(velocityIntoWall > 0.0f)
            {
                horizontalVelocity -= wallNormal * velocityIntoWall;
            }

            rigidBody.Velocity.x = horizontalVelocity.x;
            rigidBody.Velocity.z = horizontalVelocity.z;

            Vector3 horizontalForce(rigidBody.Force.x, 0.0f, rigidBody.Force.z);
            const float forceIntoWall = dot(horizontalForce, wallNormal);
            if(forceIntoWall > 0.0f)
            {
                horizontalForce -= wallNormal * forceIntoWall;
                rigidBody.Force.x = horizontalForce.x;
                rigidBody.Force.z = horizontalForce.z;
            }

            Vector3 horizontalImpulse(rigidBody.Impulse.x, 0.0f, rigidBody.Impulse.z);
            const float impulseIntoWall = dot(horizontalImpulse, wallNormal);
            if(impulseIntoWall > 0.0f)
            {
                horizontalImpulse -= wallNormal * impulseIntoWall;
                rigidBody.Impulse.x = horizontalImpulse.x;
                rigidBody.Impulse.z = horizontalImpulse.z;
            }

            entity.modified<RigidBody>();
        }

        void PreventSteepSlopeClimb(ECS::Entity entity)
        {
            auto& state = GetState();
            if(!entity.is_alive() || !entity.has<RigidBody>() || !entity.has<Transform>())
            {
                return;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            auto previousPositionIt = state.PreviousPositions.find(entityId);
            auto actorIt = state.Actors.find(entityId);
            if(previousPositionIt == state.PreviousPositions.end() || actorIt == state.Actors.end() || actorIt->second == nullptr)
            {
                return;
            }

            auto& rigidBody = entity.get_mut<RigidBody>();
            auto& transform = entity.get_mut<Transform>();
            const Vector3 previousPosition = previousPositionIt->second;
            Vector3 displacement = transform.Position - previousPosition;
            Vector3 horizontalDisplacement(displacement.x, 0.0f, displacement.z);
            const float horizontalDistance = length(horizontalDisplacement);
            if(horizontalDistance <= 1e-4f)
            {
                return;
            }

            Vector3 obstacleNormal(0.0f);
            const float probeDistance = horizontalDistance + 0.08f;
            if(!QuerySteepObstacleAhead(entity, horizontalDisplacement, probeDistance, obstacleNormal))
            {
                return;
            }

            Vector3 wallNormal(obstacleNormal.x, 0.0f, obstacleNormal.z);
            const float wallNormalLength = length(wallNormal);
            if(wallNormalLength <= 1e-4f)
            {
                return;
            }
            wallNormal /= wallNormalLength;

            Vector3 correctedHorizontal = horizontalDisplacement;
            const float intoWall = dot(correctedHorizontal, wallNormal);
            if(intoWall > 0.0f)
            {
                correctedHorizontal -= wallNormal * intoWall;
            }

            const bool previousSynchronizing = state.IsSynchronizing;
            state.IsSynchronizing = true;
            transform.Position.x = previousPosition.x + correctedHorizontal.x;
            transform.Position.z = previousPosition.z + correctedHorizontal.z;
            if(transform.Position.y > previousPosition.y)
            {
                transform.Position.y = previousPosition.y;
            }
            transform.Update();
            entity.modified<Transform>();
            state.IsSynchronizing = previousSynchronizing;

            Vector3 horizontalVelocity(rigidBody.Velocity.x, 0.0f, rigidBody.Velocity.z);
            const float velocityIntoWall = dot(horizontalVelocity, wallNormal);
            if(velocityIntoWall > 0.0f)
            {
                horizontalVelocity -= wallNormal * velocityIntoWall;
            }
            rigidBody.Velocity.x = horizontalVelocity.x;
            rigidBody.Velocity.z = horizontalVelocity.z;
            rigidBody.Velocity.y = std::min(rigidBody.Velocity.y, 0.0f);
            entity.modified<RigidBody>();

            actorIt->second->setGlobalPose(ToPxTransform(transform), true);
            if(PxRigidDynamic* dynamicActor = actorIt->second->is<PxRigidDynamic>())
            {
                PxVec3 linearVelocity = dynamicActor->getLinearVelocity();
                linearVelocity.x = rigidBody.Velocity.x;
                linearVelocity.z = rigidBody.Velocity.z;
                linearVelocity.y = std::min(linearVelocity.y, 0.0f);
                dynamicActor->setLinearVelocity(linearVelocity, false);
            }
        }

        void SyncActorFromEntity(ECS::Entity entity)
        {
            auto& state = GetState();
            const uint64 entityId = static_cast<uint64>(entity.id());
            ReleaseActor(entityId);

            if(!entity.is_alive() || !entity.has<Transform>() || !entity.has<ColliderComponent>())
            {
                return;
            }

            auto& transform = entity.get_mut<Transform>();
            auto& collider = entity.get_mut<ColliderComponent>();

            PxRigidActor* actor = nullptr;
            if(entity.has<RigidBody>())
            {
                auto& rigidBody = entity.get_mut<RigidBody>();
                PxRigidDynamic* dynamicActor = state.Physics->createRigidDynamic(ToPxTransform(transform));
                dynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rigidBody.IsKinematic);
                dynamicActor->setLinearDamping(rigidBody.LinearDamping);
                dynamicActor->setAngularDamping(rigidBody.AngularDamping);
                dynamicActor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
                dynamicActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rigidBody.HasGravity);
                ApplyAxisConstraints(rigidBody, *dynamicActor);
                dynamicActor->setLinearVelocity(ToPx(rigidBody.Velocity), false);
                dynamicActor->setAngularVelocity(ToPx(rigidBody.AngularVelocity), false);
                actor = dynamicActor;
            }
            else
            {
                actor = state.Physics->createRigidStatic(ToPxTransform(transform));
            }

            if(actor == nullptr)
            {
                return;
            }

            actor->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entityId));

            PxMaterial* material = state.DefaultMaterial;
            if(entity.has<RigidBody>())
            {
                auto& rigidBody = entity.get_mut<RigidBody>();
                material = state.Physics->createMaterial(rigidBody.Friction, rigidBody.Friction, rigidBody.Bounciness);
            }

            PxShape* shape = CreateShape(collider, transform, *state.Physics, *material);
            if(shape == nullptr)
            {
                if(material != state.DefaultMaterial)
                {
                    material->release();
                }
                actor->release();
                return;
            }

            actor->attachShape(*shape);
            shape->release();

            if(material != state.DefaultMaterial)
            {
                material->release();
            }

            if(PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>())
            {
                if(entity.has<RigidBody>())
                {
                    auto& rigidBody = entity.get_mut<RigidBody>();
                    if(!rigidBody.IsKinematic)
                    {
                        PxRigidBodyExt::setMassAndUpdateInertia(*dynamicActor, rigidBody.Mass);
                    }
                }
            }

            state.Scene->addActor(*actor);
            state.Actors[entityId] = actor;
        }

        void SyncDirtyActors()
        {
            auto& state = GetState();
            if(state.DirtyEntities.empty())
            {
                return;
            }

            std::vector<uint64> dirty(state.DirtyEntities.begin(), state.DirtyEntities.end());
            state.DirtyEntities.clear();

            for(uint64 entityId : dirty)
            {
                ECS::Entity entity = GetEntityById(entityId);
                if(entity.is_alive())
                {
                    SyncActorFromEntity(entity);
                }
                else
                {
                    ReleaseActor(entityId);
                }
            }
        }

        void MarkDirty(ECS::Entity entity)
        {
            if(entity.is_alive() && !GetState().IsSynchronizing)
            {
                GetState().DirtyEntities.insert(static_cast<uint64>(entity.id()));
            }
        }

        void PushComponentStateToActor(ECS::Entity entity, PxRigidActor& actor)
        {
            if(!entity.has<RigidBody>())
            {
                actor.setGlobalPose(ToPxTransform(entity.get<Transform>()), true);
                return;
            }

            auto& rigidBody = entity.get_mut<RigidBody>();
            PxRigidDynamic* dynamicActor = actor.is<PxRigidDynamic>();
            if(dynamicActor == nullptr)
            {
                return;
            }

            dynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rigidBody.IsKinematic);
            dynamicActor->setLinearDamping(rigidBody.LinearDamping);
            dynamicActor->setAngularDamping(rigidBody.AngularDamping);
            dynamicActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rigidBody.HasGravity);
            ApplyAxisConstraints(rigidBody, *dynamicActor);

            if(rigidBody.IsKinematic)
            {
                dynamicActor->setKinematicTarget(ToPxTransform(entity.get<Transform>()));
            }
            else
            {
                PxRigidBodyExt::setMassAndUpdateInertia(*dynamicActor, std::max(rigidBody.Mass, 0.0001f));
                dynamicActor->setLinearVelocity(ToPx(rigidBody.Velocity), false);
                dynamicActor->setAngularVelocity(ToPx(rigidBody.AngularVelocity), false);

                if(length(rigidBody.Force) > 0.0f)
                {
                    dynamicActor->addForce(ToPx(rigidBody.Force), PxForceMode::eFORCE, true);
                }
                if(length(rigidBody.Impulse) > 0.0f)
                {
                    dynamicActor->addForce(ToPx(rigidBody.Impulse), PxForceMode::eIMPULSE, true);
                }
                if(length(rigidBody.Torque) > 0.0f)
                {
                    dynamicActor->addTorque(ToPx(rigidBody.Torque), PxForceMode::eFORCE, true);
                }
            }
        }

        void PullActorStateToComponents(ECS::Entity entity, PxRigidActor& actor)
        {
            if(!entity.is_alive() || !entity.has<Transform>())
            {
                return;
            }

            Transform& transform = entity.get_mut<Transform>();
            const PxTransform pose = actor.getGlobalPose();
            auto& state = GetState();
            const bool previousSynchronizing = state.IsSynchronizing;
            state.IsSynchronizing = true;
            transform.Position = FromPx(pose.p);
            transform.RotationQuat = FromPx(pose.q);
            transform.Matrix = translate(Matrix4(1.0f), transform.Position) * mat4_cast(transform.RotationQuat) * scale(Matrix4(1.0f), transform.LocalScale);
            transform.DecompileMatrix();
            entity.modified<Transform>();

            if(PxRigidDynamic* dynamicActor = actor.is<PxRigidDynamic>())
            {
                if(entity.has<RigidBody>())
                {
                    RigidBody& rigidBody = entity.get_mut<RigidBody>();
                    rigidBody.Velocity = FromPx(dynamicActor->getLinearVelocity());
                    rigidBody.AngularVelocity = FromPx(dynamicActor->getAngularVelocity());
                    rigidBody.IsSleeping = dynamicActor->isSleeping();
                    rigidBody.Reset();
                    entity.modified<RigidBody>();
                }
            }

            state.IsSynchronizing = previousSynchronizing;
        }
    }

    bool PhysXSystem::LookAt(ECS::Entity entity, const Vector3& target)
    {
        if(!entity.is_alive() || !entity.has<Transform>())
        {
            return false;
        }

        auto& state = GetState();
        auto& transform = entity.get_mut<Transform>();

        const bool previousSynchronizing = state.IsSynchronizing;
        state.IsSynchronizing = true;
        transform.LookAt(target);
        entity.modified<Transform>();
        state.IsSynchronizing = previousSynchronizing;

        const uint64 entityId = static_cast<uint64>(entity.id());
        auto actorIt = state.Actors.find(entityId);
        if(actorIt == state.Actors.end() || actorIt->second == nullptr)
        {
            return true;
        }

        PxRigidActor* actor = actorIt->second;
        if(PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>())
        {
            if(entity.has<RigidBody>())
            {
                auto& rigidBody = entity.get_mut<RigidBody>();
                dynamicActor->setAngularVelocity(PxVec3(0.0f), false);

                if(rigidBody.IsKinematic)
                {
                    dynamicActor->setKinematicTarget(ToPxTransform(transform));
                }
                else
                {
                    dynamicActor->setGlobalPose(ToPxTransform(transform), true);
                    dynamicActor->wakeUp();
                }

                entity.modified<RigidBody>();
            }
        }
        else
        {
            actor->setGlobalPose(ToPxTransform(transform), true);
        }

        return true;
    }

    Vector3 PhysXSystem::GetGravity()
    {
        return GetState().Gravity;
    }

    void PhysXSystem::SetGravity(const Vector3& gravity)
    {
        auto& state = GetState();
        state.Gravity = gravity;
        if(state.Scene != nullptr)
        {
            state.Scene->setGravity(ToPx(gravity));
        }
    }

    bool PhysXSystem::RotateTowards(ECS::Entity entity, const Vector3& target, float maxDegreesDelta)
    {
        if(!entity.is_alive() || !entity.has<Transform>())
        {
            return false;
        }

        auto& state = GetState();
        auto& transform = entity.get_mut<Transform>();

        Quaternion nextRotation;
        if(!ComputeRotateTowardsRotation(transform, target, maxDegreesDelta, nextRotation))
        {
            return true;
        }

        const bool previousSynchronizing = state.IsSynchronizing;
        state.IsSynchronizing = true;
        transform.SetRotation(nextRotation);
        entity.modified<Transform>();
        state.IsSynchronizing = previousSynchronizing;

        const uint64 entityId = static_cast<uint64>(entity.id());
        auto actorIt = state.Actors.find(entityId);
        if(actorIt == state.Actors.end() || actorIt->second == nullptr)
        {
            return true;
        }

        PxRigidActor* actor = actorIt->second;
        if(PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>())
        {
            if(entity.has<RigidBody>())
            {
                auto& rigidBody = entity.get_mut<RigidBody>();
                dynamicActor->setAngularVelocity(PxVec3(0.0f), false);

                if(rigidBody.IsKinematic)
                {
                    dynamicActor->setKinematicTarget(ToPxTransform(transform));
                }
                else
                {
                    dynamicActor->setGlobalPose(ToPxTransform(transform), true);
                    dynamicActor->wakeUp();
                }

                entity.modified<RigidBody>();
            }
        }
        else
        {
            actor->setGlobalPose(ToPxTransform(transform), true);
        }

        return true;
    }

    void PhysXSystem::Initialize()
    {
        auto& state = GetState();

        PxTolerancesScale scale;
        scale.length = 1.0f;
        scale.speed = 9.81f;

        state.Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, state.Allocator, state.ErrorCallback);
        if(state.Foundation == nullptr)
        {
            WD_CORE_ERROR("Failed to initialize PhysX foundation.");
            return;
        }

        state.Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *state.Foundation, scale, false, nullptr, nullptr);
        if(state.Physics == nullptr)
        {
            WD_CORE_ERROR("Failed to initialize PhysX physics.");
            return;
        }

        PxInitExtensions(*state.Physics, nullptr);

        state.Dispatcher = PxDefaultCpuDispatcherCreate(2);
        PxSceneDesc sceneDesc(scale);
        sceneDesc.gravity = ToPx(state.Gravity);
        sceneDesc.cpuDispatcher = state.Dispatcher;
        sceneDesc.filterShader = FilterShader;
        sceneDesc.simulationEventCallback = &GetSimulationCallback();
        state.Scene = state.Physics->createScene(sceneDesc);
        state.DefaultMaterial = state.Physics->createMaterial(0.5f, 0.5f, 0.0f);

        ECS::World.observer<Transform, ColliderComponent>("PhysXCreateActorObserver").event(flecs::OnAdd).each([&](ECS::Entity entity, Transform&, ColliderComponent&)
        {
            MarkDirty(entity);
        });

        ECS::World.observer<ColliderComponent>("PhysXUpdateColliderObserver").event(flecs::OnSet).each([&](ECS::Entity entity, ColliderComponent&)
        {
            MarkDirty(entity);
        });

        ECS::World.observer<Transform, ColliderComponent, RigidBody>("PhysXCreateDynamicActorObserver").event(flecs::OnAdd).each([&](ECS::Entity entity, Transform&, ColliderComponent&, RigidBody&)
        {
            MarkDirty(entity);
        });

        ECS::World.observer<ColliderComponent>("PhysXRemoveColliderObserver").event(flecs::OnRemove).each([&](ECS::Entity entity, ColliderComponent&)
        {
            ReleaseActor(static_cast<uint64>(entity.id()));
        });

        ECS::World.observer<RigidBody>("PhysXRigidBodyRemovedObserver").event(flecs::OnRemove).each([&](ECS::Entity entity, RigidBody&)
        {
            MarkDirty(entity);
        });

        ECS::World.system("PhysXFixedUpdateSystem").kind<ECS::OnFixedUpdate>().each([&]
        {
            if(!EditorSimulation::ShouldRunRuntimeSystems() || state.Scene == nullptr)
            {
                return;
            }

            SyncDirtyActors();

            for(auto& [entityId, actor] : state.Actors)
            {
                ECS::Entity entity = GetEntityById(entityId);
                if(!entity.is_alive() || actor == nullptr || !entity.has<Transform>())
                {
                    continue;
                }

                state.PreviousPositions[entityId] = entity.get<Transform>().Position;
                PreventSteepSlopeAcceleration(entity);
                PushComponentStateToActor(entity, *actor);
            }

            state.Scene->simulate(Time::FixedDeltaTime);
            state.Scene->fetchResults(true);

            for(auto& [entityId, actor] : state.Actors)
            {
                ECS::Entity entity = GetEntityById(entityId);
                if(entity.is_alive() && actor != nullptr)
                {
                    PullActorStateToComponents(entity, *actor);
                    if(entity.has<RigidBody>())
                    {
                        auto& rigidBody = entity.get_mut<RigidBody>();
                        rigidBody.IsGrounded = QueryGrounded(entity);
                        entity.modified<RigidBody>();
                        PreventSteepSlopeClimb(entity);
                    }
                }
            }
        });

        IsInitialized = true;
    }

    void PhysXSystem::Deinitialize()
    {
        auto& state = GetState();

        for(auto& [entityId, actor] : state.Actors)
        {
            if(state.Scene != nullptr && actor != nullptr)
            {
                state.Scene->removeActor(*actor);
                actor->release();
            }
        }

        state.Actors.clear();
        state.PreviousPositions.clear();
        state.DirtyEntities.clear();

        if(state.DefaultMaterial != nullptr)
        {
            state.DefaultMaterial->release();
            state.DefaultMaterial = nullptr;
        }
        if(state.Scene != nullptr)
        {
            state.Scene->release();
            state.Scene = nullptr;
        }
        if(state.Dispatcher != nullptr)
        {
            state.Dispatcher->release();
            state.Dispatcher = nullptr;
        }
        if(state.Physics != nullptr)
        {
            PxCloseExtensions();
            state.Physics->release();
            state.Physics = nullptr;
        }
        if(state.Foundation != nullptr)
        {
            state.Foundation->release();
            state.Foundation = nullptr;
        }

        IsInitialized = false;
    }
}

#else

namespace Waldem
{
    bool PhysXSystem::LookAt(ECS::Entity entity, const Vector3& target)
    {
        if(!entity.is_alive() || !entity.has<Transform>())
        {
            return false;
        }

        auto& transform = entity.get_mut<Transform>();
        transform.LookAt(target);
        entity.modified<Transform>();
        return true;
    }

    bool PhysXSystem::RotateTowards(ECS::Entity entity, const Vector3& target, float maxDegreesDelta)
    {
        if(!entity.is_alive() || !entity.has<Transform>())
        {
            return false;
        }

        auto& transform = entity.get_mut<Transform>();
        Quaternion nextRotation;
        if(!ComputeRotateTowardsRotation(transform, target, maxDegreesDelta, nextRotation))
        {
            return true;
        }

        transform.SetRotation(nextRotation);
        entity.modified<Transform>();
        return true;
    }

    void PhysXSystem::Initialize()
    {
        WD_CORE_WARN("PhysXSystem was compiled without PhysX support. Set PHYSX_INCLUDE_DIR and PHYSX_LIB_DIR before generating projects to enable it.");
        IsInitialized = true;
    }

    void PhysXSystem::Deinitialize()
    {
        IsInitialized = false;
    }
}

#endif
