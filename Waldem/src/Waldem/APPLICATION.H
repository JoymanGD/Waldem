#pragma once

#include "Core.h"
#include "Application.h"

namespace Waldem
{
	class WALDEM_API Application
	{
	public:
		Application();

		void Run();
	};

	//to be defined in CLIENT
	Application* CreateApplication();
}