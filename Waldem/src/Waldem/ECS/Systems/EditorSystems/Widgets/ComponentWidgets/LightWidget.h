#pragma once
#include "Waldem/ECS/Systems/EditorSystems/WidgetSystem.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Light.h"

namespace Waldem
{
    class WALDEM_API LightWidget : public IWidgetSystem
    {
        const char* lightTypeNames[4] = { "Directional", "Point", "Spot", "Area" };
    public:
        LightWidget(ecs::Manager* eCSManager) : IWidgetSystem(eCSManager) {}

        String GetName() override { return "Light"; }
        bool IsVisible() override { return ECSManager->EntitiesWith<Light, Selected>().Count() > 0; }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, light, selected] : ECSManager->EntitiesWith<Light, Selected>())
            {
                ImGui::Combo("Type", (int*)&light.Type, lightTypeNames, 4);
                ImGui::SliderFloat3("Color", &light.Color.x, 0.0f, 1.0f);
                ImGui::DragFloat("Intensity", &light.Intensity, .5f, 0.0f, 200.0f);

                if(light.Type == LightType::Point)
                {
                    ImGui::DragFloat("Radius", &light.Radius, .5f, 0.0f, 1000.0f);
                }
                else if(light.Type == LightType::Spot)
                {
                    ImGui::DragFloat("Length", &light.Radius, .5f, 0.0f, 1000.0f);
                    // ImGui::SliderFloat("Inner Angle", &light.InnerCone, 1.0f, light.OuterCone);
                    ImGui::DragFloat("Size", &light.OuterCone, .5f, 1.0f, 90.0f);
                    ImGui::DragFloat("Softness", &light.Softness, .001f, 0.001f, 1.0f);
                }
                else if(light.Type == LightType::Area)
                {
                    ImGui::DragFloat("Width", &light.AreaWidth, .5f, 0.0f, 1000.0f);
                    ImGui::DragFloat("Height", &light.AreaHeight, .5f, 0.0f, 1000.0f);
                    ImGui::Checkbox("Two-sided", &light.AreaTwoSided);
                }
            }
        }
    };
}
