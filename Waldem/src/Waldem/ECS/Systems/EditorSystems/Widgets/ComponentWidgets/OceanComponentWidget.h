#pragma once
#include "Waldem/ECS/Systems/EditorSystems/WidgetSystem.h"
#include "Waldem/ECS/Components/Ocean.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API OceanComponentWidget : public IWidgetSystem
    {
        
    public:
        OceanComponentWidget(ecs::Manager* eCSManager) : IWidgetSystem(eCSManager) {}

        String GetName() override { return "Ocean"; }
        bool IsVisible() override { return ECSManager->EntitiesWith<Ocean, Selected>().Count() > 0; }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, ocean, selected] : ECSManager->EntitiesWith<Ocean, Selected>())
            {
                ImGui::SliderFloat("Amplitude scaling factor", &ocean.A, 0.0f, 50.0f);
                ImGui::SliderFloat("Wind Speed", &ocean.V, 0.0f, 150.0f);
                ImGui::SliderFloat("Gravitational constant", &ocean.G, 0.0f, 10.0f);
                ImGui::SliderFloat2("Wind direction", (float*)&ocean.W, 0.1f, 1.0f);
                ImGui::SliderFloat("Wave height", &ocean.WaveHeight, 1.f, 50.0f);
                ImGui::SliderFloat("Wave choppiness", &ocean.WaveChoppiness, 1.f, 50.0f);
                ImGui::SliderFloat("Normal strength", &ocean.NormalStrength, .1f, 1000.0f);
                ImGui::SliderFloat("Simulation speed", &ocean.SimulationSpeed, 0.f, 100.0f);
            }
        }
    };
}
