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
                    ImGui::Combo("Light type", (int*)&light.Type, lightTypeNames, 4);
                    ImGui::SliderFloat3("LightColor", &light.Color.x, 0.0f, 1.0f);
                    ImGui::SliderFloat("LightIntensity", &light.Intensity, 0.0f, 50.0f);

                    if(light.Type == LightType::Point)
                    {
                        ImGui::SliderFloat("LightRadius", &light.Radius, 0.0f, 20.0f);
                    }
                    else if(light.Type == LightType::Spot)
                    {
                        ImGui::SliderFloat("LightRadius", &light.Radius, 0.0f, 20.0f);
                        ImGui::SliderFloat("LightInnerAngle", &light.InnerCone, 1.0f, light.OuterCone);
                        ImGui::SliderFloat("LightOuterAngle", &light.OuterCone, light.InnerCone, 90.0f);
                        ImGui::SliderFloat("LightSharpness", &light.Sharpness, 0.001f, 1.0f);
                    }
                    // else if(light.Data.Type == LightType::Area)
                    // {
                    //     ImGui::SliderFloat("LightWidth", &light.Data.Width, 0.0f, 50.0f);
                    //     ImGui::SliderFloat("LightHeight", &light.Data.Height, 0.0f, 50.0f);
                    // }
                    ImGui::End();
                }
            }
        }
    };
}
