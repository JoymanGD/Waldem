#include "Application.h"
#include  "Waldem/Events/ApplicationEvent.h"
#include  "Waldem/Log.h"

namespace Waldem
{
	Application::Application()
	{

	}

	void Application::Run()
	{
		WindowResizeEvent e(1280, 720);
		WD_TRACE(e);
		
		while (true);
	}
} 