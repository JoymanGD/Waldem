#pragma once
#include "Waldem/ECS/Components/Ocean.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API OceanComponentWidget : public ComponentWidget<Ocean>
    {
        
    public:
        OceanComponentWidget(ECSManager* eCSManager) : ComponentWidget(eCSManager) {}

        WString GetName() override { return "Ocean"; }
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager) override {}

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, ocean, selected] : Manager->EntitiesWith<Ocean, Selected>())
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
