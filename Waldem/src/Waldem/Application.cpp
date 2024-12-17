#include <wdpch.h>
#include "Application.h"

#include "Renderer/Renderer.h"
#include "Waldem/Log.h"
#include <numeric>

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

		CurrentRenderer = {};

		CurrentRenderer.Initialize(Window.get());
	}
	
	void Application::OpenScene(Scene* scene)
	{
		CurrentRenderer.Begin();
		
		SceneData sceneData = { &CurrentRenderer, Window.get() };
		scene->Initialize(&sceneData);
		
		CurrentRenderer.End();
        
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

	
	float Application::CalculateAverageFPS(float deltaTime)
	{
		if (FrameTimes.size() < MaxFrames)
		{
			FrameTimes.push_back(deltaTime);
		}
		else
		{
			FrameTimes[FrameCount % MaxFrames] = deltaTime;
		}
		FrameCount++;

		float averageDeltaTime = std::accumulate(FrameTimes.begin(), FrameTimes.end(), 0.0f) / FrameTimes.size();
		
		return 1.0f / averageDeltaTime;
	}

	void Application::Run()
	{
		auto lastFrameTime = std::chrono::high_resolution_clock::now();

		while (IsRunning)
		{
			auto currentFrameTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> deltaTimeDuration = currentFrameTime - lastFrameTime;
			float deltaTime = deltaTimeDuration.count();
			lastFrameTime = currentFrameTime;

			SceneData sceneData = { &CurrentRenderer };
			
			CurrentScene->UpdateInternal(&sceneData, deltaTime);

			CurrentRenderer.Begin();
			CurrentScene->DrawInternal(&sceneData, deltaTime);
			CurrentRenderer.End();
			
			CurrentRenderer.Present();

			for (Layer* layer : LayerStack)
			{
				layer->OnUpdate();
			}

			Window->OnUpdate();

			float FPS = CalculateAverageFPS(deltaTime);
			Window->SetTitle(std::to_string(FPS).substr(0, 4));
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		IsRunning = false;
		return true;
	}
} 