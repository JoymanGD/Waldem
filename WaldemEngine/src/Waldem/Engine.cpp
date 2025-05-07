#include <wdpch.h>
#include "Engine.h"
#include "Renderer/Renderer.h"
#include "Waldem/Log/Log.h"
#include <numeric>
#include <SDL.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include "Time.h"
#include "Audio/Audio.h"
#include "SceneManagement/Scenes/RenderingTestScene.h"

namespace Waldem
{
	Engine* Engine::Instance = nullptr;
	
	Engine::Engine()
	{
		Initialize();
	}

	void Engine::Initialize()
	{
		PlatformInitializer::Initialize();
		Audio::Create();

		MonoRuntime = Mono();
		// MonoRuntime.Initialize();
		
		CoreECSManager = {};
		ResourceManager = {};

		WD_CORE_ASSERT(!Instance, "Application already exists!")
		Instance = this;
		
		Window = CWindow::Create();
		Window->SetEventCallback(BIND_EVENT_FN(Engine::OnEvent));

		CurrentRenderer = {};
		CurrentRenderer.Initialize(Window);

		Editor = new EditorLayer(Window, &CoreECSManager, &ResourceManager);
		PushOverlay(Editor);
		
		// Game = new GameLayer(Window, &CoreECSManager, &ResourceManager);
		// PushLayer(Game);
		
		// Debug = new DebugLayer(Window, &CoreECSManager, &ResourceManager);
		// PushLayer(Debug);

		InitializeLayers();
		
		// OpenScene(new RenderingTestScene());
	}

	void Engine::InitializeLayers()
	{
#ifdef WD_EDITOR
		Editor->Initialize();
#endif
		
#ifdef WD_GAME
		Game->Initialize(&sceneData);
#endif
		
		// Debug->Initialize(&sceneData);
	}

	void Engine::OpenScene(GameScene* scene)
	{
		Editor->OpenScene(scene);
	}

	Engine::~Engine()
	{
	}

	void Engine::PushLayer(Layer* layer)
	{
		LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Engine::PushOverlay(Layer* overlay)
	{
		LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Engine::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Engine::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Engine::OnWindowResize));

		for (int i = LayerStack.Num() - 1; i >= 0; i--)
		{
			if(LayerStack[i]->Initialized)
			{
				LayerStack[i]->OnEvent(e);
		
				if(e.Handled)
				{
					break;
				}
			}
		}
	}
	
	float Engine::CalculateAverageFPS(float deltaTime)
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

	void Engine::Run()
	{
		auto lastFrameTime = std::chrono::high_resolution_clock::now();
		auto startTime = std::chrono::high_resolution_clock::now();

		Time::FixedDeltaTime = 1.0f / 60.0f;
		float accumulatedTime = 0.0f;

		while (IsRunning)
		{
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

			for (Layer* layer : LayerStack)
			{
				if(layer->Initialized)
				{
					layer->OnUpdate(Time::DeltaTime);
				}
			}
			
			Renderer::Begin();
			for (Layer* layer : LayerStack)
			{
				if(layer->Initialized)
				{
					layer->OnDraw(Time::DeltaTime);
				}
			}
			Renderer::End();

			Renderer::BeginUI();
			for (Layer* layer : LayerStack)
			{
				if(layer->Initialized)
				{
					layer->OnDrawUI(Time::DeltaTime);
				}
			}
            Renderer::EndUI();
			
			Renderer::Present();

			if(SwapchainResizeTriggered)
			{
				CurrentRenderer.ResizeSwapchain(NewSwapchainSize);
				SwapchainResizeTriggered = false;
			}

			float FPS = CalculateAverageFPS(Time::DeltaTime);
			Window->SetTitle(std::to_string(FPS).substr(0, 4));
			
			Window->OnUpdate();
		}
	}

	bool Engine::OnWindowClose(WindowCloseEvent& e)
	{
		IsRunning = false;
		return true;
	}

	bool Engine::OnWindowResize(WindowResizeEvent& e)
	{
		auto size = Vector2(e.GetWidth(), e.GetHeight());
		SwapchainResizeTriggered = true;
		NewSwapchainSize = size;
		return true;
	}
}
