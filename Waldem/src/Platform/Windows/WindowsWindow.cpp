#include <wdpch.h>

#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/MouseEvent.h"
#include "Waldem/Events/KeyEvent.h"
#include "WindowsWindow.h"

#include "glad/glad.h"
#include "Waldem/Log.h"

namespace Waldem {

	static uint8_t s_GLFWWindowCount = 0;
	static void GLFWErrorCallback(int error, const char* description)
	{
		WD_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		Data.Title = props.Title;
		Data.Width = props.Width;
		Data.Height = props.Height;

		WD_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			int success = glfwInit();
			WD_CORE_ASSERT(success, "Could not initialize GLFW!");

			glfwSetErrorCallback(GLFWErrorCallback);
		}

		Window = glfwCreateWindow((int)props.Width, (int)props.Height, Data.Title.c_str(), nullptr, nullptr);
		++s_GLFWWindowCount;

		glfwMakeContextCurrent(Window);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		WD_CORE_ASSERT(status, "Failed to initialize Glad!");
		glfwSetWindowUserPointer(Window, &Data);
		SetVSync(true);

// Set GLFW callbacks //////////////////////////////
////////////////////////////////////////////////////
		glfwSetWindowSizeCallback(Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(Window, [](GLFWwindow* window, int key, int scancode, int action, int modes)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch(action)
			{
			case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
			case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
			case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
			default:
				break;
			}
		});

		glfwSetMouseButtonCallback(Window, [](GLFWwindow* window, int button, int action, int modes)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch(action)
			{
			case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
			case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
			default:
				break;
			}
		});

		glfwSetScrollCallback(Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(Window, [](GLFWwindow* window, double xPosition, double yPosition)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPosition, (float)yPosition);
			data.EventCallback(event);
		});
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(Window);
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return Data.VSync;
	}

}