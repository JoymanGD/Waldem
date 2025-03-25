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

        String GetName() override { return "Transform"; }
        bool IsVisible() override { return ECSManager->EntitiesWith<Transform, Selected>().Count() > 0; }

        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, transform, selected] : ECSManager->EntitiesWith<Transform, Selected>())
            {
                Vector3 cachedPosition = transform.Position;
                Quaternion cachedRotation = transform.Rotation;
                Vector3 cachedScale = transform.LocalScale;
                
                ImGui::DragFloat3("Position", &transform.Position.x);
                ImGui::DragFloat3("Rotation", &transform.Rotation.x);
                ImGui::DragFloat3("Scale", &transform.LocalScale.x);

                if(cachedPosition != transform.Position || cachedRotation != transform.Rotation || cachedScale != transform.LocalScale)
                {
                    transform.Update();
                }
            }
        }
    };
}
