#pragma once
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
        }
    };
}