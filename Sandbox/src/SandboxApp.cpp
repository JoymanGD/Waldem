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
		if(Waldem::Input::IsKeyPressed(WD_KEY_TAB))
		{
			WD_TRACE("TAB was pressed");
		}
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
	}

	~Sandbox()
	{

	}
};

Waldem::Application* Waldem::CreateApplication()
{
	return new Sandbox();
}