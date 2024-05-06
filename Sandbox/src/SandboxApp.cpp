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
		WD_INFO("ExampleLayer::Update");
	}

	void OnEvent(Waldem::Event& event) override
	{
		WD_TRACE("{0}", event);
	}
};

class Sandbox : public Waldem::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{

	}
};

Waldem::Application* Waldem::CreateApplication()
{
	return new Sandbox();
}