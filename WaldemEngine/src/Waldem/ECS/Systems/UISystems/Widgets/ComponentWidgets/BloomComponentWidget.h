#pragma once
#include "ComponentWidget.h"
#include "Waldem/ECS/Components/BloomPostProcess.h"
#include "Waldem/ECS/Systems/UISystems/Widgets/WidgetSystem.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API BloomComponentWidget : public ComponentWidget<BloomPostProcess>
    {
    public:
        BloomComponentWidget(ECSManager* eCSManager) : ComponentWidget(eCSManager) {}
        
        void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override {}

        WString GetName() override { return "Bloom"; }

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
