#include <wdpch.h>
#include "Application.h"
#include "glad/glad.h"
#include "Waldem/Log.h"
#include "Input.h"

namespace Waldem
{
	
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::Instance = nullptr;
	
	Application::Application()
	{
		WD_CORE_ASSERT(!Instance, "Application already exists!");
		Instance = this;
		Window = std::unique_ptr<Waldem::Window>(Window::Create());
		Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		ImGuiLayer = new Waldem::ImGuiLayer();
		PushOverlay(ImGuiLayer);
	}

	Application::~Application()
	{
	}

	void Application::PushLayer(Layer* layer)
	{
		LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		
		// WD_CORE_TRACE("{0}", e);

		for(auto it = LayerStack.end(); it != LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);

			if(e.Handled)
			{
				break;
			}
		}
	}

	void Application::Run()
	{
		while (IsRunning)
		{
			glClearColor(0,1,1,1);
			glClear(GL_COLOR_BUFFER_BIT);

			for(Layer* layer : LayerStack)
			{
				layer->OnUpdate();
			}

			ImGuiLayer->Begin();
			{
				for (Layer* layer : LayerStack)
					layer->OnImGuiRender();
			}
			ImGuiLayer->End();
			
			Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		IsRunning = false;
		return true;
	}
} 