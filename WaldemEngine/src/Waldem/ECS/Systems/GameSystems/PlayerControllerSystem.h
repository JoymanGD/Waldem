#pragma once
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/PlayerController.h"
#include "glm/glm.hpp"

namespace Waldem
{
    class WALDEM_API PlayerControllerSystem : public ISystem
    {
        Vector3 DeltaPos = { 0, 0, 0 };
        
    public:
        PlayerControllerSystem() {}
        
        void Initialize(InputManager* inputManager) override
        {
            inputManager->SubscribeToKeyEvent(W, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(0, 0, 1) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(S, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(0, 0, -1) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(A, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(-1, 0, 0) * multiplier;
            });
            inputManager->SubscribeToKeyEvent(D, [&](bool isPressed) 
            {
                float multiplier = isPressed ? 1.0f : -1.0f;
                DeltaPos += Vector3(1, 0, 0) * multiplier;
            });
            
            ECS::World.system<Transform, PlayerController>("PlayerControllerSystem").kind(flecs::OnUpdate).each([&](flecs::entity entity, Transform& transform, PlayerController& playerController)
            {
                if(length(DeltaPos) > 1e-6f)
                {
                    auto cameraTransform = ECS::World.lookup("EditorCamera").get<Transform>();
                    Vector3 forward = cameraTransform->GetForwardVector();
                    forward.y = 0;
                    forward = normalize(forward);
                    Vector3 right = cameraTransform->GetRightVector();
                    right.y = 0;
                    right = normalize(right);
                    Vector3 worldDir = normalize(forward * DeltaPos.z + right * DeltaPos.x);
                    transform.Move(worldDir * Time::DeltaTime * playerController.MovementSpeed);
                    entity.modified<Transform>();

                    if(playerController.RotateTowardMovementDirection)
                    {
                        Vector3 lookAtDir = mix(transform.GetForwardVector(), worldDir, playerController.RotationSpeed);
                        transform.LookAt(transform.Position + lookAtDir);
                    }
                }
            });
        }
    };
}
