#pragma once

#include "Core.h"
#include "Window.h"
#include "Layers/ImGuiLayer.h"
#include "Layers/GameLayer.h"
#include "Renderer/Renderer.h"
#include "SceneManagement/SceneManager.h"
#include "Waldem/Layers/LayerStack.h"
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
		Window& GetWindow() { return *Window; }
		void OpenScene(Scene* scene);

		//Singleton
		static Application* Instance;
		
		inline static float DeltaTime;
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		
		std::unique_ptr<Window> Window;
		Renderer CurrentRenderer;
		bool IsRunning = true;
		LayerStack LayerStack;
		ImGuiLayer* UILayer;
		GameLayer* CurrentGameLayer;

		std::vector<float> FrameTimes;
		int FrameCount = 0;
		const int MaxFrames = 100;

		float CalculateAverageFPS(float deltaTime);
	};

	//to be defined in CLIENT
	Application* CreateApplication();
}