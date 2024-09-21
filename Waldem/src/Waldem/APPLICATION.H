#pragma once

#include "Core.h"
#include "Window.h"
#include "ImGui/ImGuiLayer.h"
#include "Renderer/Renderer.h"
#include "SceneManagement/SceneManager.h"
#include "Waldem/LayerStack.h"
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
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		
		std::unique_ptr<Window> Window;
		Renderer CurrentRenderer;
		bool IsRunning = true;
		LayerStack LayerStack;
		ImGuiLayer* ImGuiLayer;
        Scene* CurrentScene;
	protected:
	};

	//to be defined in CLIENT
	Application* CreateApplication();
}