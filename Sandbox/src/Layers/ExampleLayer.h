#pragma once
#include "imgui/imgui.h"
#include "Waldem/Layer.h"

namespace Sandbox
{
    class ExampleLayer : public Waldem::Layer
    {
    public:
        ExampleLayer() : Layer("Example")
        {
        }

        void OnUpdate() override
        {
        }

        void OnEvent(Waldem::Event& event) override
        {
        }

        void OnUIRender() override
        {
            ImGui::Begin("Test");
            ImGui::Text("Hello world");
            ImGui::End();
        }
    };
}