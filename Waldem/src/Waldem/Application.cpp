#include <wdpch.h>
#include "Application.h"

#include "GLFW/glfw3.h"
#include "Renderer/Renderer.h"
#include "Waldem/Log.h"

namespace Waldem
{
	
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::Instance = nullptr;
	
	Application::Application()
	{
		WD_CORE_ASSERT(!Instance, "Application already exists!")
		Instance = this;
		Window = std::unique_ptr<Waldem::Window>(Window::Create());
		Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		ImGuiLayer = new Waldem::ImGuiLayer();
		PushOverlay(ImGuiLayer);

		Renderer::Initialize();
	}
	
	void Application::OpenScene(Scene* scene)
	{
		scene->Initialize();
        
		CurrentScene = scene;
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
		float lastFrameTime = (float)glfwGetTime();

		while (IsRunning)
		{
			float currentFrameTime = (float)glfwGetTime();
			float deltaTime = currentFrameTime - lastFrameTime;
			lastFrameTime = currentFrameTime;

			CurrentScene->UpdateInternal(deltaTime);

			CurrentScene->DrawInternal();

			for (Layer* layer : LayerStack)
			{
				layer->OnUpdate();
			}

			ImGuiLayer->Begin();
			for (Layer* layer : LayerStack)
			{
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