#include <wdpch.h>
#include "Application.h"
#include "Renderer/Renderer.h"
#include "Waldem/Log/Log.h"
#include <numeric>
#include <SDL.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include "Time.h"
#include "Audio/Audio.h"

namespace Waldem
{
	
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::Instance = nullptr;
	
	Application::Application()
	{
		Initialize();
	}

	void Application::Initialize()
	{
		PlatformInitializer::Initialize();
		Audio::Create();

		MonoRuntime = Mono();
		// MonoRuntime.Initialize();
		
		CoreECSManager = {};
		ResourceManager = {};

		WD_CORE_ASSERT(!Instance, "Application already exists!")
		Instance = this;
		
		Window = Window::Create();
		Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		CurrentRenderer = {};
		CurrentRenderer.Initialize(Window);

		Renderer::Begin();
		
		Vector2 resolution = Vector2(Window->GetWidth(), Window->GetHeight());
		ResourceManager.CreateRenderTarget("TargetRT", resolution.x, resolution.y, TextureFormat::R8G8B8A8_UNORM);
		
		Editor = new EditorLayer(Window, &CoreECSManager, &ResourceManager);
		PushOverlay(Editor);
		
		Game = new GameLayer(Window, &CoreECSManager, &ResourceManager);
		PushLayer(Game);
		
		Debug = new DebugLayer(Window, &CoreECSManager, &ResourceManager);
		PushLayer(Debug);
		
		Renderer::End();
	}

	void Application::InitializeLayers()
	{
		SceneData sceneData = { Window };
		Editor->Initialize(&sceneData);
		Game->Initialize(&sceneData);
		Debug->Initialize(&sceneData);
	}

	void Application::OpenScene(GameScene* scene)
	{
		// SceneData sceneData = { Window };
		// Game->OpenScene(scene, &sceneData);
		SceneData sceneData = { Window };
		Editor->OpenScene(scene, &sceneData);
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
			if((*--it)->Initialized)
			{
				(*--it)->OnEvent(e);

				if(e.Handled)
				{
					break;
				}
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
		auto startTime = std::chrono::high_resolution_clock::now();

		Time::FixedDeltaTime = 1.0f / 60.0f;
		float accumulatedTime = 0.0f;

		while (IsRunning)
		{
			Window->OnUpdate();

			auto currentFrameTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> deltaTimeDuration = currentFrameTime - lastFrameTime;
			Time::DeltaTime = deltaTimeDuration.count();
			std::chrono::duration<float> elapsedTimeDuration = currentFrameTime - startTime;
			Time::ElapsedTime = elapsedTimeDuration.count();
			lastFrameTime = currentFrameTime;

			// Accumulate unprocessed time
			accumulatedTime += Time::DeltaTime;

			// Run FixedUpdate() as many times as needed
			while (accumulatedTime >= Time::FixedDeltaTime)
			{
				// 🔁 Your fixed-timestep physics update
				for (Layer* layer : LayerStack)
				{
					if(layer->Initialized)
					{
						layer->OnFixedUpdate(Time::FixedDeltaTime);
					}
				}

				accumulatedTime -= Time::FixedDeltaTime;
			}

			Renderer::Begin();
			
			for (Layer* layer : LayerStack)
			{
				if(layer->Initialized)
				{
					layer->OnUpdate(Time::DeltaTime);
				}
			}
			
			for (Layer* layer : LayerStack)
			{
				if(layer->Initialized)
				{
					layer->OnDraw(Time::DeltaTime);
				}
			}

			Editor->Begin();
			for (Layer* layer : LayerStack)
			{
				if(layer->Initialized)
				{
					layer->OnDrawUI(Time::DeltaTime);
				}
			}
			Editor->End();
			
			Renderer::End();
			
			Renderer::Present();

			float FPS = CalculateAverageFPS(Time::DeltaTime);
			Window->SetTitle(std::to_string(FPS).substr(0, 4));
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		IsRunning = false;
		return true;
	}
}