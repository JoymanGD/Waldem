#include <wdpch.h>
#include "Application.h"
#include "glad/glad.h"
#include "Waldem/Log.h"
#include "Input.h"

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

		ImGuiLayer = new Waldem::ImGuiLayer();
		PushOverlay(ImGuiLayer);

		TrisVA.reset(VertexArray::Create());

		//vertex buffer
		float vertices[3*7] = {
			-.5f, -.5f, .0f, 1.0f, 0.0f, 1.0f, 1.0f,
			.5f, -.5f, .0f, 0.0f, 1.0f, 1.0f, 1.0f,
			0.f, 0.5f, .0f, 1.0f, 1.0f, 0.0f, 1.0f,
		};
		std::shared_ptr<VertexBuffer> trisVB;
		trisVB.reset(VertexBuffer::Create(vertices, sizeof(vertices)));
		trisVB->SetLayout
		({
			{ ShaderDataType::Float3, "Position", false },
			{ ShaderDataType::Float4, "Color", false },
		});
		TrisVA->AddVertexBuffer(trisVB);

		uint32_t indices[3] = { 0, 1, 2 };
		std::shared_ptr<IndexBuffer> trisIB;
		trisIB.reset(IndexBuffer::Create(indices, 3));
		TrisVA->SetIndexBuffer(trisIB);

		std::string vertexSrc = R"(
			#version 330 core

			layout(location = 0) in vec3 Position;
			layout(location = 1) in vec4 Color;
			out vec3 OutPosition;
			out vec4 OutColor;

			void main()
			{
				OutColor = Color;
				OutPosition = Position;
				gl_Position = vec4(Position, 1.0);
			}

		)";
		
		std::string fragmentSrc = R"(
			#version 330 core

			layout(location = 0) out vec4 Color;
			in vec3 OutPosition;
			in vec4 OutColor;

			void main()
			{
				//Color = vec4((OutPosition+1)/2, 1);
				Color = OutColor;
			}

		)";
		TrisShader.reset(new Shader(vertexSrc, fragmentSrc));

		SquareVA.reset(VertexArray::Create());

		//vertex buffer
		
		float squareVertices[3*4] = {
			-.5f, -.5f, .0f,
			.5f, -.5f, .0f,
			.5f, .5f, .0f,
			-.5f, .5f, .0f,
		};
		
		std::shared_ptr<VertexBuffer> squareVB;
		squareVB.reset(VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

		squareVB->SetLayout
		({
			{ ShaderDataType::Float3, "Position", false }
		});

		SquareVA->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
		std::shared_ptr<IndexBuffer> squareIB;
		squareIB.reset(IndexBuffer::Create(squareIndices, 6));
		SquareVA->SetIndexBuffer(squareIB);

		std::string squareVertexSrc = R"(
			#version 330 core

			layout(location = 0) in vec3 Position;
			out vec3 OutPosition;

			void main()
			{
				OutPosition = Position;
				gl_Position = vec4(Position, 1.0);
			}

		)";
		
		std::string squareFragmentSrc = R"(
			#version 330 core

			layout(location = 0) out vec4 Color;
			in vec3 OutPosition;

			void main()
			{
				Color = vec4((OutPosition+1)/2, 1);
			}

		)";
		SquareShader.reset(new Shader(squareVertexSrc, squareFragmentSrc));
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

	void Application::Run()
	{
		while (IsRunning)
		{
			glClearColor(0.1f,0.1f,0.1f,1);
			glClear(GL_COLOR_BUFFER_BIT);

			SquareShader->Bind();
			SquareVA->Bind();
			glDrawElements(GL_TRIANGLES, SquareVA->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);

			TrisShader->Bind();
			TrisVA->Bind();
			glDrawElements(GL_TRIANGLES, TrisVA->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);

			for(Layer* layer : LayerStack)
			{
				layer->OnUpdate();
			}

			ImGuiLayer->Begin();
			for (Layer* layer : LayerStack)
			{
				layer->OnImGuiRender();
			}
			ImGuiLayer->End();
			
			Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		IsRunning = false;
		return true;
	}
} 