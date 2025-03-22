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
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, light, selected] : ECSManager->EntitiesWith<Light, Selected>())
            {
                if (ImGui::Begin("Light"))
                {
                    ImGui::Combo("Type", (int*)&light.Type, lightTypeNames, 4);
                    ImGui::SliderFloat3("Color", &light.Color.x, 0.0f, 1.0f);
                    ImGui::SliderFloat("Intensity", &light.Intensity, 0.0f, 50.0f);

                    if(light.Type == LightType::Point)
                    {
                        ImGui::SliderFloat("Radius", &light.Radius, 0.0f, 20.0f);
                    }
                    else if(light.Type == LightType::Spot)
                    {
                        ImGui::SliderFloat("Length", &light.Radius, 0.0f, 20.0f);
                        // ImGui::SliderFloat("Inner Angle", &light.InnerCone, 1.0f, light.OuterCone);
                        ImGui::SliderFloat("Size", &light.OuterCone, 1.0f, 90.0f);
                        ImGui::SliderFloat("Softness", &light.Softness, 0.001f, 1.0f);
                    }
                    else if(light.Type == LightType::Area)
                    {
                        ImGui::SliderFloat("Width", &light.AreaWidth, 0.0f, 1000.0f);
                        ImGui::SliderFloat("Height", &light.AreaHeight, 0.0f, 1000.0f);
                        ImGui::Checkbox("Two-sided", &light.AreaTwoSided);
                    }
                    ImGui::End();
                }
            }
        }
    };
}
