#pragma once

#include "Core.h"
#include "Window.h"

namespace Waldem
{
	class WALDEM_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	private:
		std::unique_ptr<Window> Window;
		bool IsRunning = true;
	};

	//to be defined in CLIENT
	Application* CreateApplication();
}
