#pragma once

#include "Core.h"
#include "PlatformInitializer.h"
#include "Window.h"
#include "Layers/DebugLayer.h"
#include "Layers/EditorLayer.h"
#include "Layers/GameLayer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Resources/ResourceManager.h"
#include "Waldem/Layers/LayerStack.h"
#include "Waldem/Events/Event.h"
#include "Waldem/Events/ApplicationEvent.h"

namespace Waldem
{
	class WALDEM_API Application
	{
	public:
		Application();
		void Initialize();
		virtual ~Application();
		void Run();
		void OnEvent(Event& e);
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		Window& GetWindow() { return *Window; }
		void OpenScene(Scene* scene);

		//Singleton
		static Application* Instance;
	private:
		bool OnWindowClose(WindowCloseEvent& e);

		Window* Window;
		Renderer CurrentRenderer;
        ecs::Manager CoreECSManager;
		ResourceManager ResourceManager;
		bool IsRunning = true;
		LayerStack LayerStack;
		std::vector<float> FrameTimes;
		int FrameCount = 0;
		const int MaxFrames = 100;

		//Layers
		EditorLayer* Editor;
		DebugLayer* Debug;
		GameLayer* Game;

		float CalculateAverageFPS(float deltaTime);
	};

	//to be defined in CLIENT
	Application* CreateApplication();
}