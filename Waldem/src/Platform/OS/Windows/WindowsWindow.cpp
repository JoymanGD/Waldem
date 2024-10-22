#include <wdpch.h>

#include "WindowsWindow.h"
#include "Platform/Graphics/OpenGL/OpenGLContext.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/MouseEvent.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Log.h"

#if defined(WALDEM_WINDOW_API_SDL)
#include <SDL.h>
#include <SDL_syswm.h>
#elif defined(WALDEM_WINDOW_API_GLFW)
#endif

namespace Waldem
{
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
    
#if defined(WALDEM_WINDOW_API_SDL)
    HWND WindowsWindow::GetWindowsHandle() const
    {
        HWND hwnd = nullptr;
        
        // Obtain the HWND from SDL_Window using SDL_SysWMinfo
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version); // Initialize SDL version info
        if (SDL_GetWindowWMInfo(Window, &wmInfo))
        {
            hwnd = wmInfo.info.win.window;  // Get the native HWND
        }
        else
        {
            WD_CORE_ERROR("Failed to get SDL window info! SDL_Error: {0}", SDL_GetError());
        }

        return hwnd;
    }

    void WindowsWindow::Init(const WindowProps& props)
    {
        Data.Title = props.Title;
        Data.Width = props.Width;
        Data.Height = props.Height;
        Data.VSync = false; // Default VSync is off

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            WD_CORE_ERROR("SDL could not initialize! SDL_Error: {0}", SDL_GetError());
            return;
        }

        // Create an SDL window
        Window = SDL_CreateWindow(Data.Title.c_str(),
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    Data.Width, Data.Height,
                                    SDL_WINDOW_RESIZABLE);
        if (!Window)
        {
            WD_CORE_ERROR("Window could not be created! SDL_Error: {0}", SDL_GetError());
            return;
        }
    }

    void WindowsWindow::Shutdown()
    {
        SDL_DestroyWindow(Window);
        SDL_Quit();
    }

    void WindowsWindow::ProcessEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                {
                    WindowCloseEvent closeEvent;
                    Data.EventCallback(closeEvent);  // Trigger the event callback
                    break;
                }
            case SDL_KEYDOWN:
                WD_CORE_INFO("Key Pressed: {0}", SDL_GetKeyName(event.key.keysym.sym));
                break;
            case SDL_MOUSEBUTTONDOWN:
                WD_CORE_INFO("Mouse Button Pressed: {0}", (int)event.button.button);
                break;
            default:
                break;
            }
        }
    }

    void WindowsWindow::OnUpdate()
    {
        ProcessEvents();
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        Data.VSync = enabled;
    }
	
#elif defined(WALDEM_WINDOW_API_GLFW)

	static uint8_t s_GLFWWindowCount = 0;
	static void GLFWErrorCallback(int error, const char* description)
	{
		WD_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	HWND WindowsWindow::GetWindowsHandle() const
	{
		return nullptr;
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		Data.Title = props.Title;
		Data.Width = props.Width;
		Data.Height = props.Height;

    	if (s_GLFWWindowCount == 0)
    	{
    		int success = glfwInit();
    		WD_CORE_ASSERT(success, "Could not initialize GLFW!");

    		glfwSetErrorCallback(GLFWErrorCallback);
    	}

    	Window = glfwCreateWindow((int)props.Width, (int)props.Height, Data.Title.c_str(), nullptr, nullptr);
    	++s_GLFWWindowCount;
		
    	Context = new OpenGLContext(Window);
    	Context->Init();
    	WD_CORE_INFO("{0} info:", Context->GetContextName());
    	Context->LogContextInfo();
		
    	glfwSetWindowUserPointer(Window, &Data);
    	SetVSync(true);

// Set GLFW callbacks //////////////////////////////
////////////////////////////////////////////////////
		glfwSetWindowSizeCallback(Window, [](GLFWwindow* window, int width, int height)
		{
			float widthf = static_cast<float>(width);
			float heightf = static_cast<float>(height);
			
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = widthf;
			data.Height = heightf;

			WindowResizeEvent event(heightf, heightf);
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

		glfwSetCharCallback(Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			KeyTypedEvent event(keycode);
			data.EventCallback(event);
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
		Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		Data.VSync = enabled;
	}
#endif
}
