#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/WidgetSystem.h"

namespace Waldem
{
    class WALDEM_API ColliderComponentWidget : public IWidgetSystem
    {
    private:
        const char* colliderTypeNames[5] = { "None", "Sphere", "Box", "Capsule", "Mesh" };
    public:
        ColliderComponentWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}

        String GetName() override { return "Collider"; }
        bool IsVisible() override { return Manager->EntitiesWith<ColliderComponent, Selected>().Count() > 0; }

        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [entity, collider, selected] : Manager->EntitiesWith<ColliderComponent, Selected>())
            {
                ImGui::Combo("Type", (int*)&collider.Type, colliderTypeNames, 5);

                switch (collider.Type)
                {
                case WD_COLLIDER_TYPE_BOX:
                    {
                        ImGui::DragFloat3("Size", &collider.BoxData.Size.x, 1.f, 0.1f, 1000.0f);
                        break;
                    }
                case WD_COLLIDER_TYPE_SPHERE:
                    {
                        ImGui::DragFloat("Radius", &collider.SphereData.Radius, 1.f, 0.1f, 1000.0f);
                        break;
                    }
                case WD_COLLIDER_TYPE_CAPSULE:
                    {
                        ImGui::DragFloat("Radius", &collider.CapsuleData.Radius, 1.f, 0.1f, 1000.0f);
                        ImGui::DragFloat("Height", &collider.CapsuleData.Height, 1.f, 0.1f, 1000.0f);
                        break;
                    }
                case WD_COLLIDER_TYPE_MESH:
                    {
                        break;
                    }
                default: ;
                }
            }
        }
    };
}
