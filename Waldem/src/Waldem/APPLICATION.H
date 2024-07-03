#pragma once

#include "Core.h"
#include "Window.h"
#include "ImGui/ImGuiLayer.h"
#include "Renderer/Buffer.h"
#include "Renderer/VertexArray.h"
#include "Waldem/LayerStack.h"
#include "Waldem/Events/Event.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Renderer/Shader.h"

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
	public:
		static Application* Instance;
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		
		std::unique_ptr<Window> Window;
		bool IsRunning = true;
		LayerStack LayerStack;
		ImGuiLayer* ImGuiLayer;

		std::shared_ptr<Shader> TrisShader;
		std::shared_ptr<VertexArray> TrisVA;
		
		std::shared_ptr<Shader> SquareShader;
		std::shared_ptr<VertexArray> SquareVA;
	};

	//to be defined in CLIENT
	Application* CreateApplication();
}
