#include <wdpch.h>

#include "WindowsWindow.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/MouseEvent.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Log.h"
#include <SDL.h>
#include <SDL_syswm.h>

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
    
    HWND WindowsWindow::GetWindowsHandle() const
    {
        HWND hwnd = nullptr;
        
        // Obtain the HWND from SDL_Window using SDL_SysWMinfo
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version) // Initialize SDL version info
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
}
