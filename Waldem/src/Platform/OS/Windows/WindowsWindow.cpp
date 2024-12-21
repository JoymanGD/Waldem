#include <wdpch.h>

#include "WindowsWindow.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/MouseEvent.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Log.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include "backends/imgui_impl_sdl2.h"

namespace Waldem
{
    Window* Window::Create(const WindowProps& props)
    {
    	Instance = new WindowsWindow(props);
        
        return Instance;
    }

    void* Window::GetNativeWindow()
    {
        auto windowsWindow = dynamic_cast<WindowsWindow*>(Instance);
        
        return windowsWindow->NativeWindow;
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
        
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version)
        if (SDL_GetWindowWMInfo(NativeWindow, &wmInfo))
        {
            hwnd = wmInfo.info.win.window;
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
        Data.Position = props.Position;
        Data.Width = props.Width;
        Data.Height = props.Height;
        Data.VSync = false;

        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            WD_CORE_ERROR("SDL could not initialize! SDL_Error: {0}", SDL_GetError());
            return;
        }

        NativeWindow = SDL_CreateWindow(Data.Title.c_str(),
                                    Data.Position.x,
                                    Data.Position.y,
                                    Data.Width, Data.Height,
                                    SDL_WINDOW_RESIZABLE);
        if (!NativeWindow)
        {
            WD_CORE_ERROR("Window could not be created! SDL_Error: {0}", SDL_GetError());
        }
    }

    void WindowsWindow::Shutdown()
    {
        SDL_DestroyWindow(NativeWindow);
        SDL_Quit();
    }

    void WindowsWindow::ProcessEvents()
    {
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent))
        {
            ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
            
            switch (sdlEvent.type)
            {
            case SDL_QUIT:
                {
                    WindowCloseEvent event = {};
                    Data.EventCallback(event);
                    break;
                }
            case SDL_KEYDOWN:
                {
                    if(sdlEvent.key.state == 1 && sdlEvent.key.repeat == 0)
                    {
                        KeyPressedEvent event(sdlEvent.key.keysym.sym, 0);
                        Data.EventCallback(event);
                        WD_CORE_INFO("Key Pressed: {0}", SDL_GetKeyName(sdlEvent.key.keysym.sym), sdlEvent.key.repeat);
                    }
                    break;
                }
            case SDL_KEYUP:
                {
                    if(sdlEvent.key.state == 0 && sdlEvent.key.repeat == 0)
                    {
                        KeyReleasedEvent event(sdlEvent.key.keysym.sym);
                        Data.EventCallback(event);
                        WD_CORE_INFO("Key Released: {0}", SDL_GetKeyName(sdlEvent.key.keysym.sym));
                    }
                    
                    break;
                }
            case SDL_MOUSEBUTTONDOWN:
                {
                    MouseButtonPressedEvent event(sdlEvent.button.button);
                    Data.EventCallback(event);
                    WD_CORE_INFO("Mouse Button Pressed: {0}", (int)sdlEvent.button.button);
                    break;
                }
            case SDL_MOUSEBUTTONUP:
                {
                    MouseButtonReleasedEvent event(sdlEvent.button.button);
                    Data.EventCallback(event);
                    WD_CORE_INFO("Mouse Button Released: {0}", (int)sdlEvent.button.button);
                    break;
                }
            case SDL_MOUSEMOTION:
                {
                    MouseMovedEvent event(sdlEvent.motion.x, sdlEvent.motion.y);
                    Data.EventCallback(event);
                    break;
                }
            case SDL_MOUSEWHEEL:
                {
                    MouseScrolledEvent event(sdlEvent.wheel.x, sdlEvent.wheel.y);
                    Data.EventCallback(event);
                    break;
                }
            default:
                break;
            }
        }
    }

    void WindowsWindow::SetTitle(String title)
    {
        SDL_SetWindowTitle(NativeWindow, title.c_str());
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
