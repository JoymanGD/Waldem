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
#include "PlatformInitializer.h"
#include "ECS/ECS.h"
#include "Input/Input.h"
#include "Renderer/Viewport/ViewportManager.h"
#include "SceneManagement/SceneManager.h"

namespace Waldem
{
	Engine* Engine::Instance = nullptr;
	
	Engine::Engine()
	{
		Initialize();
	}

	void Engine::Initialize()
	{
		WD_CORE_ASSERT(!Instance, "Application already exists!")
		Instance = this;
		
		PlatformInitializer::Initialize();

		Input::Initialize();

		AudioManager = Audio();

		MonoRuntime = Mono();
		// MonoRuntime.Initialize();

		//Window
		Window = CWindow::Create();
		Window->SetEventCallback(BIND_EVENT_FN(Engine::OnEvent));

		//Renderer
		CurrentRenderer = {};
		CurrentRenderer.Initialize(Window);

		ECS.Initialize();
		// Debug = new DebugLayer(Window, &CoreECSManager, &ResourceManager);
		// PushLayer(Debug);
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
			SceneManager::CheckRequests();
			
			Window->Begin();
			
			auto currentFrameTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> deltaTimeDuration = currentFrameTime - lastFrameTime;
			Time::DeltaTime = deltaTimeDuration.count();
			std::chrono::duration<float> elapsedTimeDuration = currentFrameTime - startTime;
			Time::ElapsedTime = elapsedTimeDuration.count();
			lastFrameTime = currentFrameTime;

			accumulatedTime += Time::DeltaTime;

			while (accumulatedTime >= Time::FixedDeltaTime)
			{
				ECS::RunFixedUpdatePipeline(Time::FixedDeltaTime);

				accumulatedTime -= Time::FixedDeltaTime;
			}

			ECS::RunUpdatePipeline(Time::DeltaTime);

			for (auto layer : LayerStack)
			{
				layer->Draw();
			}

			Renderer::Present();

			ViewportManager::GetMainViewport()->ApplyPendingResize();

			float FPS = CalculateAverageFPS(Time::DeltaTime);
			Window->SetTitle(std::to_string(FPS).substr(0, 4));

			Window->End();
		}
	}

	bool Engine::OnWindowClose(WindowCloseEvent& e)
	{
		IsRunning = false;
		ECS::World.release();
		return true;
	}

	bool Engine::OnWindowResize(WindowResizeEvent& e)
	{
		auto size = Point2(e.GetWidth(), e.GetHeight());
		ViewportManager::GetMainViewport()->RequestResize(size);

		return true;
	}
}
