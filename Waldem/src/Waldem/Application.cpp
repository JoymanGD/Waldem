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
		CoreECSManager = {};
		ResourceManager = {};

		WD_CORE_ASSERT(!Instance, "Application already exists!")
		Instance = this;
		Window = Window::Create();
		Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		CurrentRenderer = {};
		CurrentRenderer.Initialize(Window);
		
		UILayer = new EditorLayer(Window, &CoreECSManager, &ResourceManager);
		PushOverlay(UILayer);
		
		CurrentGameLayer = new GameLayer(Window, &CoreECSManager, &ResourceManager);
		PushLayer(CurrentGameLayer);
	}
	
	void Application::OpenScene(Scene* scene)
	{
		SceneData sceneData = { Window };
		CurrentGameLayer->OpenScene(scene, &sceneData);
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
			Window->OnUpdate();

			auto currentFrameTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> deltaTimeDuration = currentFrameTime - lastFrameTime;
			DeltaTime = deltaTimeDuration.count();
			lastFrameTime = currentFrameTime;

			Renderer::Begin();
			
			for (Layer* layer : LayerStack)
			{
				layer->OnUpdate(DeltaTime);
			}

			UILayer->Begin();
			for (Layer* layer : LayerStack)
			{
				layer->OnDrawUI(DeltaTime);
			}
			UILayer->End();
			
			Renderer::End();
			
			Renderer::Present();

			float FPS = CalculateAverageFPS(DeltaTime);
			Window->SetTitle(std::to_string(FPS).substr(0, 4));
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		IsRunning = false;
		return true;
	}
} 