#pragma once

#include "Core.h"
#include "PlatformInitializer.h"
#include "Window.h"
#include "ECS/ECSManager.h"
#include "Layers/DebugLayer.h"
#include "Layers/EditorLayer.h"
#include "Layers/GameLayer.h"
#include "Renderer/Renderer.h"
#include "Resources/ResourceManager.h"
#include "Scripting/Mono.h"
#include "Waldem/Layers/LayerStack.h"
#include "Waldem/Events/Event.h"
#include "Waldem/Events/ApplicationEvent.h"

namespace Waldem
{
	class WALDEM_API Engine
	{
	public:
		Engine();
		void Initialize();
		void InitializeLayers();
		virtual ~Engine();
		void Run();
		void OnEvent(Event& e);
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		CWindow& GetWindow() { return *Window; }
		void OpenScene(GameScene* scene);

		//Singleton
		static Engine* Instance;
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		CWindow* Window;
		Renderer CurrentRenderer;
        ECSManager CoreECSManager;
		ResourceManager ResourceManager;
		Mono MonoRuntime;
		bool IsRunning = true;
		LayerStack LayerStack;
		std::vector<float> FrameTimes;
		int FrameCount = 0;
		const int MaxFrames = 100;

		bool SwapchainResizeTriggered = false;
		Point2 NewSwapchainSize = {};

		//Layers
		EditorLayer* Editor;
		DebugLayer* Debug;
		GameLayer* Game;

		float CalculateAverageFPS(float deltaTime);
	};

	//to be defined in CLIENT
	Engine* CreateApplication();
}