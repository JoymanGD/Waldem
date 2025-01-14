#pragma once
#include "imgui.h"
#include "System.h"
#include "Waldem/ECS/Components/Ocean.h"

namespace Waldem
{
    class WALDEM_API OceanSimulationEditorSystem : ISystem
    {
        
    public:
        OceanSimulationEditorSystem(ecs::Manager* eCSManager) : ISystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
        }

        void Update(float deltaTime) override
        {
            if (ImGui::Begin("Ocean Simulation Editor"))
            {
                for (auto [entity, ocean] : ECSManager->EntitiesWith<Ocean>())
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
                ImGui::End();
            }
        }
    };
}