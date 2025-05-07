#pragma once
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Components/Light.h"

namespace Waldem
{
    class WALDEM_API LightComponentWidget : public ComponentWidget<Light>
    {
    private:
        const char* lightTypeNames[4] = { "Directional", "Point", "Spot", "Area" };
    public:
        LightComponentWidget(ECSManager* eCSManager) : ComponentWidget(eCSManager) {}

        WString GetName() override { return "Light"; }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, light, selected] : Manager->EntitiesWith<Light, Selected>())
            {
                ImGui::Combo("Type", (int*)&light.Data.Type, lightTypeNames, 4);
                ImGui::ColorEdit3("Color", &light.Data.Color.x);
                ImGui::DragFloat("Intensity", &light.Data.Intensity, .5f, 0.0f, 200.0f);

                if(light.Data.Type == LightType::Point)
                {
                    ImGui::DragFloat("Radius", &light.Data.Radius, .5f, 0.0f, 1000.0f);
                }
                else if(light.Data.Type == LightType::Spot)
                {
                    ImGui::DragFloat("Length", &light.Data.Radius, .5f, 0.0f, 1000.0f);
                    // ImGui::SliderFloat("Inner Angle", &light.InnerCone, 1.0f, light.OuterCone);
                    ImGui::DragFloat("Size", &light.Data.OuterCone, .5f, 1.0f, 90.0f);
                    ImGui::DragFloat("Softness", &light.Data.Softness, .001f, 0.001f, 1.0f);
                }
                else if(light.Data.Type == LightType::Area)
                {
                    ImGui::DragFloat("Width", &light.Data.AreaWidth, .5f, 0.0f, 1000.0f);
                    ImGui::DragFloat("Height", &light.Data.AreaHeight, .5f, 0.0f, 1000.0f);
                    ImGui::Checkbox("Two-sided", (bool*)&light.Data.AreaTwoSided);
                }
            }
        }
    };
}
