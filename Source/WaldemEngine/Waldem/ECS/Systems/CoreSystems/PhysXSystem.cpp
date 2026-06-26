#include "wdpch.h"
#include "PhysXSystem.h"

#include "Waldem/ECS/Components/CharacterController.h"

#if WD_WITH_PHYSX

#include "Waldem/Editor/EditorSimulation.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/ScriptComponent.h"
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Components/RigidBody.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Scripting/ScriptEngine.h"
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
            PxControllerManager* ControllerManager;
            std::unordered_map<uint64, PxRigidActor*> Actors;
            std::unordered_map<uint64, PxController*> Controllers;
            std::unordered_map<uint64, Vector3> PreviousPositions;
            std::unordered_map<uint64, Vector3> ColliderScales;
            std::unordered_map<uint64, Vector3> ControllerScales;
            std::unordered_map<uint64, Vector2> ControllerCapsuleSizes;
            bool IsSynchronizing = false;
            Vector3 Gravity = Vector3(0.0f, -30.00f, 0.0f);
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
            if(!entityA.is_alive() || !entityB.is_alive())
            {
                return;
            }

            const bool hasColliderA = entityA.has<ColliderComponent>();
            const bool hasColliderB = entityB.has<ColliderComponent>();

            if(hasColliderA)
            {
                auto& colliderA = entityA.get_mut<ColliderComponent>();

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
            }

            if(hasColliderB)
            {
                auto& colliderB = entityB.get_mut<ColliderComponent>();

                if(colliderB.IsTrigger)
                {
                    switch(type)
                    {
                    case CollisionEventType::Enter:
                        if(colliderB.OnTriggerEnter) colliderB.OnTriggerEnter(entityA);
                        break;
                    case CollisionEventType::Stay:
                        if(colliderB.OnTriggerStay) colliderB.OnTriggerStay(entityA);
                        break;
                    case CollisionEventType::Exit:
                        if(colliderB.OnTriggerExit) colliderB.OnTriggerExit(entityA);
                        break;
                    }
                }
                else
                {
                    switch(type)
                    {
                    case CollisionEventType::Enter:
                        if(colliderB.OnCollisionEnter) colliderB.OnCollisionEnter(entityA, manifold);
                        break;
                    case CollisionEventType::Stay:
                        if(colliderB.OnCollisionStay) colliderB.OnCollisionStay(entityA, manifold);
                        break;
                    case CollisionEventType::Exit:
                        if(colliderB.OnCollisionExit) colliderB.OnCollisionExit(entityA, manifold);
                        break;
                    }
                }
            }

            if(entityA.has<CharacterController>() && !hasColliderA && entityA.has<ScriptComponent>())
            {
                ScriptEngine::OnCollisionEvent(type, entityA, entityB, entityA.get<ScriptComponent>(), manifold);
            }

            if(entityB.has<CharacterController>() && !hasColliderB && entityB.has<ScriptComponent>())
            {
                ScriptEngine::OnCollisionEvent(type, entityB, entityA, entityB.get<ScriptComponent>(), manifold);
            }
        }

        class PhysXControllerHitReport final : public PxUserControllerHitReport
        {
        public:
            void onShapeHit(const PxControllerShapeHit& hit) override
            {
                const uint64 controllerEntityId = GetEntityId(hit.controller != nullptr ? hit.controller->getActor() : nullptr);
                const uint64 otherEntityId = GetEntityId(hit.actor);
                if(controllerEntityId == 0 || otherEntityId == 0)
                {
                    return;
                }

                ContactsManifold manifold;
                ContactPoint contactPoint;
                contactPoint.Position = Vector3(static_cast<float>(hit.worldPos.x), static_cast<float>(hit.worldPos.y), static_cast<float>(hit.worldPos.z));
                contactPoint.PositionA = contactPoint.Position;
                contactPoint.PositionB = contactPoint.Position;
                contactPoint.Normal = normalize(FromPx(hit.worldNormal));
                contactPoint.Penetration = 0.0f;
                contactPoint.CalculateBasic();
                manifold.Points.Add(contactPoint);

                DispatchCollisionEvent(controllerEntityId, otherEntityId, manifold, CollisionEventType::Stay);
            }

            void onControllerHit(const PxControllersHit&) override {}
            void onObstacleHit(const PxControllerObstacleHit&) override {}
        };

        PhysXControllerHitReport& GetControllerHitReport()
        {
            static PhysXControllerHitReport report;
            return report;
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

        PxFilterFlags FilterShader(PxFilterObjectAttributes attributes0, PxFilterData, PxFilterObjectAttributes attributes1, PxFilterData, PxPairFlags& pairFlags, const void*, PxU32)
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
            state.ColliderScales.erase(entityId);
        }

        void ReleaseController(uint64 entityId)
        {
            auto& state = GetState();
            auto it = state.Controllers.find(entityId);
            if(it == state.Controllers.end())
            {
                return;
            }

            if(it->second != nullptr)
            {
                it->second->release();
            }

            state.Controllers.erase(it);
            state.PreviousPositions.erase(entityId);
            state.ControllerScales.erase(entityId);
            state.ControllerCapsuleSizes.erase(entityId);
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

        bool CreateStaticActor(ECS::Entity entity, Transform& transform, ColliderComponent& collider)
        {
            auto& state = GetState();
            if(state.Physics == nullptr || state.Scene == nullptr || state.DefaultMaterial == nullptr)
            {
                return false;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            ReleaseActor(entityId);

            PxRigidStatic* actor = state.Physics->createRigidStatic(ToPxTransform(transform));
            if(actor == nullptr)
            {
                return false;
            }

            actor->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entityId));

            PxShape* shape = CreateShape(collider, transform, *state.Physics, *state.DefaultMaterial);
            if(shape == nullptr)
            {
                actor->release();
                return false;
            }

            actor->attachShape(*shape);
            shape->release();

            state.Scene->addActor(*actor);
            state.Actors[entityId] = actor;
            state.ColliderScales[entityId] = GetColliderScale(transform);
            return true;
        }

        bool CreateDynamicActor(ECS::Entity entity, Transform& transform, ColliderComponent& collider, RigidBody& rigidBody)
        {
            auto& state = GetState();
            if(state.Physics == nullptr || state.Scene == nullptr)
            {
                return false;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            ReleaseActor(entityId);

            PxRigidDynamic* actor = state.Physics->createRigidDynamic(ToPxTransform(transform));
            if(actor == nullptr)
            {
                return false;
            }

            actor->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entityId));
            actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rigidBody.IsKinematic);
            actor->setLinearDamping(rigidBody.LinearDamping);
            actor->setAngularDamping(rigidBody.AngularDamping);
            actor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
            actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rigidBody.HasGravity);
            ApplyAxisConstraints(rigidBody, *actor);
            actor->setLinearVelocity(ToPx(rigidBody.Velocity), false);
            actor->setAngularVelocity(ToPx(rigidBody.AngularVelocity), false);

            PxMaterial* material = state.Physics->createMaterial(rigidBody.Friction, rigidBody.Friction, rigidBody.Bounciness);
            PxShape* shape = CreateShape(collider, transform, *state.Physics, *material);
            if(shape == nullptr)
            {
                material->release();
                actor->release();
                return false;
            }

            actor->attachShape(*shape);
            shape->release();
            material->release();

            if(!rigidBody.IsKinematic)
            {
                PxRigidBodyExt::setMassAndUpdateInertia(*actor, std::max(rigidBody.Mass, 0.0001f));
            }

            state.Scene->addActor(*actor);
            state.Actors[entityId] = actor;
            state.ColliderScales[entityId] = GetColliderScale(transform);
            return true;
        }

        void ValidateActorStorage()
        {
            auto& state = GetState();
            std::vector<uint64> actorIds;
            actorIds.reserve(state.Actors.size());
            for(const auto& [entityId, actor] : state.Actors)
            {
                actorIds.push_back(entityId);
            }

            for(const uint64 entityId : actorIds)
            {
                auto actorIt = state.Actors.find(entityId);
                if(actorIt == state.Actors.end())
                {
                    continue;
                }

                PxRigidActor* actor = actorIt->second;
                ECS::Entity entity = GetEntityById(entityId);
                if(!entity.is_alive() || actor == nullptr || !entity.has<Transform>() || !entity.has<ColliderComponent>())
                {
                    ReleaseActor(entityId);
                    continue;
                }

                auto& transform = entity.get_mut<Transform>();
                auto& collider = entity.get_mut<ColliderComponent>();
                if(entity.has<RigidBody>())
                {
                    auto& rigidBody = entity.get_mut<RigidBody>();
                    if(actor->is<PxRigidDynamic>() == nullptr)
                    {
                        CreateDynamicActor(entity, transform, collider, rigidBody);
                    }
                }
                else if(actor->is<PxRigidDynamic>() != nullptr)
                {
                    CreateStaticActor(entity, transform, collider);
                }
            }
        }

        bool CreateCharacterController(ECS::Entity entity, Transform& transform, CharacterController& controller)
        {
            auto& state = GetState();
            if(state.ControllerManager == nullptr || state.DefaultMaterial == nullptr)
            {
                return false;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            ReleaseController(entityId);

            const Vector3 controllerScale = glm::abs(transform.LocalScale);
            const float radiusScale = std::max(controllerScale.x, controllerScale.z);

            PxCapsuleControllerDesc desc;
            desc.material = state.DefaultMaterial;
            desc.position = PxExtendedVec3(transform.Position.x, transform.Position.y, transform.Position.z);
            desc.height = std::max(0.0f, controller.CapsuleHeight * controllerScale.y);
            desc.radius = std::max(0.001f, controller.CapsuleRadius * radiusScale);
            desc.upDirection = PxVec3(0.0f, 1.0f, 0.0f);
            desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
            desc.contactOffset = std::max(0.02f, desc.radius * 0.1f);
            desc.stepOffset = std::min(desc.height * 0.5f, std::max(0.1f, controller.CapsuleHeight * controllerScale.y * 0.25f));
            desc.slopeLimit = cosf(PxPi / 4.0f);
            desc.reportCallback = &GetControllerHitReport();

            if(!desc.isValid())
            {
                WD_CORE_WARN("Invalid PhysX character controller descriptor for entity {}", entityId);
                return false;
            }

            PxController* createdController = state.ControllerManager->createController(desc);
            if(createdController == nullptr)
            {
                WD_CORE_WARN("Failed to create PhysX character controller for entity {}", entityId);
                return false;
            }

            if(PxRigidDynamic* actor = createdController->getActor()->is<PxRigidDynamic>())
            {
                actor->userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entityId));
            }

            state.Controllers[entityId] = createdController;
            state.PreviousPositions[entityId] = transform.Position;
            state.ControllerScales[entityId] = controllerScale;
            state.ControllerCapsuleSizes[entityId] = Vector2(controller.CapsuleHeight, controller.CapsuleRadius);
            controller.IsGrounded = false;
            controller.JumpRequested = false;
            controller.MoveVelocity = Vector3(0.0f);
            return true;
        }

        void PullControllerStateToComponents(ECS::Entity entity, PxController& controller)
        {
            if(!entity.is_alive() || !entity.has<Transform>())
            {
                return;
            }

            const PxExtendedVec3 position = controller.getPosition();
            const Vector3 controllerPosition(static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z));

            auto& state = GetState();
            auto& transform = entity.get_mut<Transform>();
            const bool previousSynchronizing = state.IsSynchronizing;
            state.IsSynchronizing = true;
            transform.PushPhysicsInterpolationState(controllerPosition, transform.RotationQuat);
            transform.Position = controllerPosition;
            transform.Update();
            entity.modified<Transform>();
            state.IsSynchronizing = previousSynchronizing;

            state.PreviousPositions[static_cast<uint64>(entity.id())] = controllerPosition;
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
            const Vector3 actorPosition = FromPx(pose.p);
            const Quaternion actorRotation = FromPx(pose.q);
            auto& state = GetState();
            const bool previousSynchronizing = state.IsSynchronizing;
            state.IsSynchronizing = true;
            transform.PushPhysicsInterpolationState(actorPosition, actorRotation);
            transform.Position = actorPosition;
            transform.RotationQuat = actorRotation;
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

    void PhysXSystem::ApplyImpulse(ECS::Entity entity, const Vector3& impulse)
    {
        if(!entity.is_alive() || !entity.has<RigidBody>())
        {
            return;
        }

        auto& rigidBody = entity.get_mut<RigidBody>();
        rigidBody.Impulse += impulse;

        auto& state = GetState();
        auto actorIt = state.Actors.find(static_cast<uint64>(entity.id()));
        if(actorIt == state.Actors.end() || actorIt->second == nullptr)
        {
            entity.modified<RigidBody>();
            return;
        }

        PxRigidDynamic* dynamicActor = actorIt->second->is<PxRigidDynamic>();
        if(dynamicActor == nullptr || rigidBody.IsKinematic)
        {
            entity.modified<RigidBody>();
            return;
        }

        dynamicActor->addForce(ToPx(impulse), PxForceMode::eIMPULSE, true);
        dynamicActor->wakeUp();
        rigidBody.Impulse = Vector3(0.0f);
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
        state.ControllerManager = PxCreateControllerManager(*state.Scene);
        state.DefaultMaterial = state.Physics->createMaterial(0.5f, 0.5f, 0.0f);

        ECS::World.observer<Transform, ColliderComponent>("PhysXCreateActorObserver").without<RigidBody>().event(flecs::OnAdd).each([&](ECS::Entity entity, Transform& transform, ColliderComponent& collider)
        {
            CreateStaticActor(entity, transform, collider);
        });

        ECS::World.observer<Transform, CharacterController>("CharacterControllerCreatedObserver").without<RigidBody>().without<ColliderComponent>().event(flecs::OnAdd).each([&](ECS::Entity entity, Transform& transform, CharacterController& controller)
        {
            CreateCharacterController(entity, transform, controller);
        });

        ECS::World.observer<CharacterController>("CharacterControllerUpdatedObserver").event(flecs::OnSet).each([&](ECS::Entity entity, CharacterController& controller)
        {
            if(state.IsSynchronizing || !entity.has<Transform>())
            {
                return;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            const auto sizeIt = state.ControllerCapsuleSizes.find(entityId);
            const bool sizeChanged = sizeIt == state.ControllerCapsuleSizes.end()
                || fabsf(sizeIt->second.x - controller.CapsuleHeight) > 1e-5f
                || fabsf(sizeIt->second.y - controller.CapsuleRadius) > 1e-5f;
            if(!sizeChanged)
            {
                return;
            }

            auto& transform = entity.get_mut<Transform>();
            CreateCharacterController(entity, transform, controller);
        });

        ECS::World.observer<Transform, ColliderComponent, RigidBody>("PhysXCreateDynamicActorObserver").event(flecs::OnAdd).each([&](ECS::Entity entity, Transform& transform, ColliderComponent& collider, RigidBody& rigidBody)
        {
            CreateDynamicActor(entity, transform, collider, rigidBody);
        });

        ECS::World.observer<ColliderComponent>("PhysXUpdateColliderObserver").event(flecs::OnSet).each([&](ECS::Entity entity, ColliderComponent& collider)
        {
            if(state.IsSynchronizing || state.Physics == nullptr)
            {
                return;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            auto actorIt = state.Actors.find(entityId);
            if(!entity.has<Transform>())
            {
                return;
            }

            Transform& transform = entity.get_mut<Transform>();
            if(entity.has<RigidBody>())
            {
                RigidBody& rigidBody = entity.get_mut<RigidBody>();
                if(actorIt == state.Actors.end() || actorIt->second == nullptr || actorIt->second->is<PxRigidDynamic>() == nullptr)
                {
                    CreateDynamicActor(entity, transform, collider, rigidBody);
                    return;
                }
            }
            else if(actorIt == state.Actors.end() || actorIt->second == nullptr)
            {
                CreateStaticActor(entity, transform, collider);
                return;
            }

            PxRigidActor* actor = actorIt->second;

            const PxU32 shapeCount = actor->getNbShapes();
            if(shapeCount > 0)
            {
                std::vector<PxShape*> shapes(shapeCount);
                actor->getShapes(shapes.data(), shapeCount);
                for(PxShape* shape : shapes)
                {
                    actor->detachShape(*shape, true);
                }
            }

            PxMaterial* material = state.DefaultMaterial;
            if(entity.has<RigidBody>())
            {
                RigidBody& rigidBody = entity.get_mut<RigidBody>();
                material = state.Physics->createMaterial(rigidBody.Friction, rigidBody.Friction, rigidBody.Bounciness);
            }

            PxShape* shape = CreateShape(collider, transform, *state.Physics, *material);
            if(shape == nullptr)
            {
                if(material != state.DefaultMaterial)
                {
                    material->release();
                }
                ReleaseActor(entityId);
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
                    RigidBody& rigidBody = entity.get_mut<RigidBody>();
                    if(!rigidBody.IsKinematic)
                    {
                        PxRigidBodyExt::setMassAndUpdateInertia(*dynamicActor, std::max(rigidBody.Mass, 0.0001f));
                    }
                }
            }

            state.ColliderScales[entityId] = GetColliderScale(transform);
        });

        ECS::World.observer<Transform>("PhysXUpdateTransformObserver").event(flecs::OnSet).each([&](ECS::Entity entity, Transform& transform)
        {
            if(state.IsSynchronizing)
            {
                return;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            auto actorIt = state.Actors.find(entityId);
            if(actorIt != state.Actors.end() && actorIt->second != nullptr)
            {
                PxRigidActor* actor = actorIt->second;

                if(entity.has<ColliderComponent>())
                {
                    const Vector3 currentScale = GetColliderScale(transform);
                    const auto scaleIt = state.ColliderScales.find(entityId);
                    const bool scaleChanged = scaleIt == state.ColliderScales.end() || length(scaleIt->second - currentScale) > 1e-4f;
                    if(scaleChanged && state.Physics != nullptr)
                    {
                        auto& collider = entity.get_mut<ColliderComponent>();

                        const PxU32 shapeCount = actor->getNbShapes();
                        if(shapeCount > 0)
                        {
                            std::vector<PxShape*> shapes(shapeCount);
                            actor->getShapes(shapes.data(), shapeCount);
                            for(PxShape* shape : shapes)
                            {
                                actor->detachShape(*shape, true);
                            }
                        }

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
                            ReleaseActor(entityId);
                            return;
                        }

                        actor->attachShape(*shape);
                        shape->release();

                        if(material != state.DefaultMaterial)
                        {
                            material->release();
                        }

                        state.ColliderScales[entityId] = currentScale;
                    }
                }

                if(PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>())
                {
                    if(entity.has<RigidBody>())
                    {
                        auto& rigidBody = entity.get_mut<RigidBody>();
                        if(rigidBody.IsKinematic)
                        {
                            dynamicActor->setKinematicTarget(ToPxTransform(transform));
                        }
                        else
                        {
                            dynamicActor->setGlobalPose(ToPxTransform(transform), true);
                            dynamicActor->wakeUp();
                        }
                    }
                }
                else
                {
                    actor->setGlobalPose(ToPxTransform(transform), true);
                }
            }

            auto controllerIt = state.Controllers.find(entityId);
            if(controllerIt != state.Controllers.end() && controllerIt->second != nullptr && !entity.has<RigidBody>() && !entity.has<ColliderComponent>())
            {
                const Vector3 currentScale = glm::abs(transform.LocalScale);
                const auto scaleIt = state.ControllerScales.find(entityId);
                const bool scaleChanged = scaleIt == state.ControllerScales.end() || length(scaleIt->second - currentScale) > 1e-4f;
                if(scaleChanged && entity.has<CharacterController>())
                {
                    auto& characterController = entity.get_mut<CharacterController>();
                    CreateCharacterController(entity, transform, characterController);
                    return;
                }

                const Vector3 previousPosition = state.PreviousPositions.contains(entityId) ? state.PreviousPositions[entityId] : transform.Position;
                const Vector3 displacement = transform.Position - previousPosition;
                if(length(displacement) > 1e-5f)
                {
                    controllerIt->second->setPosition(PxExtendedVec3(transform.Position.x, transform.Position.y, transform.Position.z));
                    state.PreviousPositions[entityId] = transform.Position;
                    if(entity.has<CharacterController>())
                    {
                        auto& controller = entity.get_mut<CharacterController>();
                        controller.JumpRequested = false;
                        controller.IsGrounded = false;
                    }
                }
            }
        });

        ECS::World.observer<RigidBody>("PhysXUpdateRigidBodyObserver").event(flecs::OnSet).each([&](ECS::Entity entity, RigidBody& rigidBody)
        {
            if(state.IsSynchronizing || state.Physics == nullptr || !entity.has<Transform>())
            {
                return;
            }

            const uint64 entityId = static_cast<uint64>(entity.id());
            auto actorIt = state.Actors.find(entityId);
            if(actorIt == state.Actors.end() || actorIt->second == nullptr)
            {
                if(entity.has<ColliderComponent>())
                {
                    auto& transform = entity.get_mut<Transform>();
                    auto& collider = entity.get_mut<ColliderComponent>();
                    CreateDynamicActor(entity, transform, collider, rigidBody);
                }
                return;
            }

            PxRigidDynamic* dynamicActor = actorIt->second->is<PxRigidDynamic>();
            if(dynamicActor == nullptr)
            {
                if(entity.has<ColliderComponent>())
                {
                    auto& transform = entity.get_mut<Transform>();
                    auto& collider = entity.get_mut<ColliderComponent>();
                    CreateDynamicActor(entity, transform, collider, rigidBody);
                }
                return;
            }

            dynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, rigidBody.IsKinematic);
            dynamicActor->setLinearDamping(rigidBody.LinearDamping);
            dynamicActor->setAngularDamping(rigidBody.AngularDamping);
            dynamicActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rigidBody.HasGravity);
            ApplyAxisConstraints(rigidBody, *dynamicActor);

            if(entity.has<ColliderComponent>())
            {
                PxMaterial* material = state.Physics->createMaterial(rigidBody.Friction, rigidBody.Friction, rigidBody.Bounciness);
                const PxU32 shapeCount = dynamicActor->getNbShapes();
                if(shapeCount > 0)
                {
                    std::vector<PxShape*> shapes(shapeCount);
                    dynamicActor->getShapes(shapes.data(), shapeCount);
                    for(PxShape* shape : shapes)
                    {
                        PxMaterial* materials[] = { material };
                        shape->setMaterials(materials, 1);
                    }
                }
                material->release();
            }

            if(rigidBody.IsKinematic)
            {
                dynamicActor->setKinematicTarget(ToPxTransform(entity.get<Transform>()));
            }
            else
            {
                PxRigidBodyExt::setMassAndUpdateInertia(*dynamicActor, std::max(rigidBody.Mass, 0.0001f));
                dynamicActor->setLinearVelocity(ToPx(rigidBody.Velocity), false);
                dynamicActor->setAngularVelocity(ToPx(rigidBody.AngularVelocity), false);
                dynamicActor->wakeUp();
            }
        });

        ECS::World.observer<ColliderComponent>("PhysXRemoveColliderObserver").event(flecs::OnRemove).each([&](ECS::Entity entity, ColliderComponent&)
        {
            ReleaseActor(static_cast<uint64>(entity.id()));
        });

        ECS::World.observer<RigidBody>("PhysXRigidBodyRemovedObserver").event(flecs::OnRemove).each([&](ECS::Entity entity, RigidBody&)
        {
            const uint64 entityId = static_cast<uint64>(entity.id());
            ReleaseActor(entityId);

            if(state.Physics == nullptr || state.Scene == nullptr || !entity.is_alive() || !entity.has<Transform>() || !entity.has<ColliderComponent>())
            {
                return;
            }

            auto& transform = entity.get_mut<Transform>();
            auto& collider = entity.get_mut<ColliderComponent>();
            CreateStaticActor(entity, transform, collider);
        });

        ECS::World.observer<CharacterController>("CharacterControllerRemovedObserver").event(flecs::OnRemove).each([&](ECS::Entity entity, CharacterController&)
        {
            ReleaseController(static_cast<uint64>(entity.id()));
        });

        ECS::World.system("PhysXFixedUpdateSystem").kind<ECS::OnFixedUpdate>().each([&]
        {
            if(!EditorSimulation::ShouldRunRuntimeSystems() || state.Scene == nullptr)
            {
                return;
            }

            ValidateActorStorage();

            for(auto& [entityId, actor] : state.Actors)
            {
                ECS::Entity entity = GetEntityById(entityId);
                if(!entity.is_alive() || actor == nullptr || !entity.has<Transform>())
                {
                    continue;
                }

                state.PreviousPositions[entityId] = entity.get<Transform>().Position;
                PushComponentStateToActor(entity, *actor);
            }

            for(auto& [entityId, controller] : state.Controllers)
            {
                ECS::Entity entity = GetEntityById(entityId);
                if(!entity.is_alive() || controller == nullptr || !entity.has<Transform>() || !entity.has<CharacterController>())
                {
                    continue;
                }

                auto& characterController = entity.get_mut<CharacterController>();

                Vector3 displacement = characterController.MoveVelocity * Time::FixedDeltaTime;

                if(characterController.JumpRequested && characterController.IsGrounded)
                {
                    characterController.MoveVelocity.y = characterController.JumpSpeed;
                    characterController.IsGrounded = false;
                }

                characterController.MoveVelocity.y += state.Gravity.y * characterController.GravityScale * Time::FixedDeltaTime;
                displacement.y += characterController.MoveVelocity.y * Time::FixedDeltaTime;

                const PxControllerCollisionFlags collisionFlags = controller->move(
                    ToPx(displacement),
                    0.001f,
                    Time::FixedDeltaTime,
                    PxControllerFilters());

                if(collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN) && characterController.MoveVelocity.y < 0.0f)
                {
                    characterController.MoveVelocity.y = 0.0f;
                }
                if(collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_UP) && characterController.MoveVelocity.y > 0.0f)
                {
                    characterController.MoveVelocity.y = 0.0f;
                }

                characterController.IsGrounded = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
                characterController.JumpRequested = false;

                PullControllerStateToComponents(entity, *controller);
            }

            state.Scene->simulate(Time::FixedDeltaTime);
            state.Scene->fetchResults(true);

            for(auto& [entityId, actor] : state.Actors)
            {
                ECS::Entity entity = GetEntityById(entityId);
                if(entity.is_alive() && actor != nullptr)
                {
                    PullActorStateToComponents(entity, *actor);
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
        for(auto& [entityId, controller] : state.Controllers)
        {
            if(controller != nullptr)
            {
                controller->release();
            }
        }
        state.Controllers.clear();
        state.PreviousPositions.clear();
        state.ColliderScales.clear();
        state.ControllerScales.clear();
        state.ControllerCapsuleSizes.clear();

        if(state.DefaultMaterial != nullptr)
        {
            state.DefaultMaterial->release();
            state.DefaultMaterial = nullptr;
        }
        if(state.ControllerManager != nullptr)
        {
            state.ControllerManager->release();
            state.ControllerManager = nullptr;
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
    void PhysXSystem::ApplyImpulse(ECS::Entity entity, const Vector3& impulse)
    {
        if(!entity.is_alive() || !entity.has<RigidBody>())
        {
            return;
        }

        auto& rigidBody = entity.get_mut<RigidBody>();
        rigidBody.Impulse += impulse;
        entity.modified<RigidBody>();
    }

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
