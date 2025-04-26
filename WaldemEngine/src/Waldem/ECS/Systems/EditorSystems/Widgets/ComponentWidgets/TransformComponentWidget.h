#pragma once
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/WidgetSystem.h"
#include "Waldem/ECS/Components/Transform.h"

namespace Waldem
{
    class WALDEM_API TransformComponentWidget : public IWidgetSystem
    {
    public:
        TransformComponentWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}

        WString GetName() override { return "Transform"; }
        bool IsVisible() override { return Manager->EntitiesWith<Transform, Selected>().Count() > 0; }

        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, transform, selected] : Manager->EntitiesWith<Transform, Selected>())
            {
                Vector3 cachedPosition = transform.Position;
                Quaternion cachedRotation = transform.Rotation;
                Vector3 cachedScale = transform.LocalScale;
                
                ImGui::DragFloat3("Position", &transform.Position.x);
                
                auto euler = transform.GetEuler();
                ImGui::DragFloat3("Rotation", &euler.x);
                transform.SetEuler(euler);
                
                ImGui::DragFloat3("Scale", &transform.LocalScale.x);

                if(cachedPosition != transform.Position || cachedRotation != transform.Rotation || cachedScale != transform.LocalScale)
                {
                    transform.Update();
                }
            }
        }
    };
}
