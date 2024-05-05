#include <wdpch.h>
#include "Application.h"
#include "Waldem/Log.h"
#include "GLFW/glfw3.h"

namespace Waldem
{
	Application::Application()
	{
		Window = std::unique_ptr<Waldem::Window>(Window::Create());
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		while (IsRunning)
		{
			glClearColor(1,0,1,1);
			glClear(GL_COLOR_BUFFER_BIT);
			Window->OnUpdate();
		}
	}
} 