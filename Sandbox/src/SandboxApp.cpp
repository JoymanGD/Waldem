#include "wdpch.h"
#include <Waldem.h>

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
};

class Sandbox : public Waldem::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		// PushOverlay(new Waldem::ImGuiLayer());
	}

	~Sandbox()
	{

	}
};

Waldem::Application* Waldem::CreateApplication()
{
	return new Sandbox();
}