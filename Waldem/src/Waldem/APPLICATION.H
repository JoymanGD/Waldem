#pragma once

#include "Core.h"

namespace Waldem
{
	class WALDEM_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	//to be declined in CLIENT
	Application* CreateApplication();
}

