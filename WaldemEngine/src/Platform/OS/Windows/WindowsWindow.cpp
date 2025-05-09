#include <wdpch.h>

#include "WindowsWindow.h"
#include <filesystem>
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/MouseEvent.h"
#include "Waldem/Events/KeyEvent.h"
#include "Waldem/Log/Log.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include "backends/imgui_impl_sdl2.h"

namespace Waldem
{
    CWindow* CWindow::Create(const WindowProps& props)
    {
    	Instance = new WindowsWindow(props);
        
        return Instance;
    }

    void* CWindow::GetNativeWindow()
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

        NativeWindow = SDL_CreateWindow(Data.Title.C_Str(),
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

    void WindowsWindow::RequestEvents()
    {
        SDL_Event* sdlEvent = new SDL_Event();
        while (SDL_PollEvent(sdlEvent))
        {
            ImGui_ImplSDL2_ProcessEvent(sdlEvent);
            SDL_Event* copiedEvent = new SDL_Event(); // Allocate default-initialized event
            *copiedEvent = *sdlEvent;                  // Copy the contents
            Events.Add(copiedEvent);
        }
    }

    void WindowsWindow::ProcessEvents()
    {
        for (auto sdlEvent : Events)
        {
            switch (sdlEvent->type)
            {
            case SDL_QUIT:
                {
                    WindowCloseEvent event = {};
                    Data.EventCallback(event);
                    break;
                }
            case SDL_WINDOWEVENT:
                switch (sdlEvent->window.event)
                {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    {
                        int newWidth = sdlEvent->window.data1;
                        int newHeight = sdlEvent->window.data2;

                        Data.Width = newWidth;
                        Data.Height = newHeight;

                        WindowResizeEvent event(newWidth, newHeight);
                        Data.EventCallback(event);

                        WD_CORE_INFO("Window Resized: {0}x{1}", newWidth, newHeight);
                        break;
                    }
                case SDL_WINDOWEVENT_MOVED:
                    {
                        int newX = sdlEvent->window.data1;
                        int newY = sdlEvent->window.data2;

                        Data.Position.x = newX;
                        Data.Position.y = newY;

                        WindowMovedEvent event(Point2(newX, newY));
                        Data.EventCallback(event);

                        WD_CORE_INFO("Window Moved: {0}:{1}", newX, newY);
                        break;
                    }
                default:
                    break;
                }
                break;
            case SDL_KEYDOWN:
                {
                    if(sdlEvent->key.state == 1 && sdlEvent->key.repeat == 0)
                    {
                        KeyPressedEvent event(sdlEvent->key.keysym.sym, 0);
                        Data.EventCallback(event);
                        WD_CORE_INFO("Key Pressed: {0}", SDL_GetKeyName(sdlEvent->key.keysym.sym), sdlEvent->key.repeat);
                    }
                    break;
                }
            case SDL_KEYUP:
                {
                    if(sdlEvent->key.state == 0 && sdlEvent->key.repeat == 0)
                    {
                        KeyReleasedEvent event(sdlEvent->key.keysym.sym);
                        Data.EventCallback(event);
                        WD_CORE_INFO("Key Released: {0}", SDL_GetKeyName(sdlEvent->key.keysym.sym));
                    }
                    
                    break;
                }
            case SDL_MOUSEBUTTONDOWN:
                {
                    MouseButtonPressedEvent event(sdlEvent->button.button);
                    Data.EventCallback(event);
                    WD_CORE_INFO("Mouse Button Pressed: {0}", (int)sdlEvent->button.button);
                    break;
                }
            case SDL_MOUSEBUTTONUP:
                {
                    MouseButtonReleasedEvent event(sdlEvent->button.button);
                    Data.EventCallback(event);
                    WD_CORE_INFO("Mouse Button Released: {0}", (int)sdlEvent->button.button);
                    break;
                }
            case SDL_MOUSEMOTION:
                {
                    MouseMovedEvent event(sdlEvent->motion.x, sdlEvent->motion.y);
                    Data.EventCallback(event);
                    break;
                }
            case SDL_MOUSEWHEEL:
                {
                    MouseScrolledEvent event(sdlEvent->wheel.x, sdlEvent->wheel.y);
                    Data.EventCallback(event);
                    break;
                }
            default:
                break;
            }

            delete sdlEvent;
        }

        Events.Clear();
    }

    void WindowsWindow::SetTitle(WString title)
    {
        SDL_SetWindowTitle(NativeWindow, title.C_Str());
    }

    void WindowsWindow::Begin()
    {
        RequestEvents();
    }

    void WindowsWindow::End()
    {
        ProcessEvents();
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        Data.VSync = enabled;
    }
}