#pragma once

#include "Core.h"
#include "Window.h"
#include "Waldem/LayerStack.h"
#include "Waldem/Events/Event.h"
#include "Waldem/Events/ApplicationEvent.h"

namespace Waldem
{
	class WALDEM_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		
		std::unique_ptr<Window> Window;
		bool IsRunning = true;
		LayerStack LayerStack;
	};

	//to be defined in CLIENT
	Application* CreateApplication();
}
