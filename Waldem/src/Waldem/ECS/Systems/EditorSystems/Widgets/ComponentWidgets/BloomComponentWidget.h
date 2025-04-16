#pragma once
#include "Waldem/ECS/Components/BloomPostProcess.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/WidgetSystem.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API BloomComponentWidget : public IWidgetSystem
    {
    public:
        BloomComponentWidget(ECSManager* eCSManager) : IWidgetSystem(eCSManager) {}
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override {}

        String GetName() override { return "Bloom"; }
        bool IsVisible() override { return Manager->EntitiesWith<BloomPostProcess, Selected>().Count() > 0; }

        void Update(float deltaTime) override
        {
            for (auto [transformEntity, bloom, selected] : Manager->EntitiesWith<BloomPostProcess, Selected>())
            {
                ImGui::SliderFloat("Bloom Threshold", &bloom.BrightThreshold, 0.0f, 2.0f);
                ImGui::SliderFloat("Bloom Intensity", &bloom.BloomIntensity, 0.0f, 10.0f);
                ImGui::SliderFloat2("Texel Size", &bloom.TexelSize.x, 0.0f, 1.0f);
            }
        }
    };
}
