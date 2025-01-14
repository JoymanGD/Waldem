#pragma once
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/EditorSystems/WidgetSystem.h"
#include "Waldem/Renderer/Model/Transform.h"

namespace Waldem
{
    class WALDEM_API TransformComponentWidget : public IWidgetSystem
    {
    public:
        TransformComponentWidget(ecs::Manager* eCSManager) : IWidgetSystem(eCSManager) {}

        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, transform, selected] : ECSManager->EntitiesWith<Transform, Selected>())
            {
                if (ImGui::Begin("Transform"))
                {
                    Vector3 cachedPosition = transform.Position;
                    Quaternion cachedRotation = transform.Rotation;
                    Vector3 cachedScale = transform.LocalScale;
                    
                    ImGui::InputFloat3("Position", &transform.Position.x);
                    ImGui::InputFloat3("Rotation", &transform.Rotation.x);
                    ImGui::InputFloat3("Scale", &transform.LocalScale.x);
                    ImGui::End();

                    if(cachedPosition != transform.Position || cachedRotation != transform.Rotation || cachedScale != transform.LocalScale)
                    {
                        transform.Update();
                    }
                }
            }
        }
    };
}
