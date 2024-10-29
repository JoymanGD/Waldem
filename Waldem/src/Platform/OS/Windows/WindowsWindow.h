#pragma once

#include "Waldem/Window.h"
#include "Waldem/Renderer/RenderingContext.h"

#define WALDEM_WINDOW_API_SDL

#if defined(WALDEM_WINDOW_API_SDL)
#include <SDL.h>
#elif defined(WALDEM_WINDOW_API_GLFW)
#include "GLFW/glfw3.h"
#endif

namespace Waldem
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void OnUpdate() override;
    
        float GetWidth() const override { return Data.Width; }
        float GetHeight() const override { return Data.Height; }
    
        void SetEventCallback(const EventCallbackFn& callback) override { Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        bool IsVSync() const override { return Data.VSync; }

        void* GetNativeWindow() const override { return Window; }
        HWND GetWindowsHandle() const override;

    private:
        void Init(const WindowProps& props);
        void Shutdown();
    
        void ProcessEvents();

        struct WindowData
        {
            std::string Title;
            float Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData Data;
#if defined(WALDEM_WINDOW_API_SDL)
        SDL_Window* Window;
#elif defined(WALDEM_WINDOW_API_GLFW)
        GLFWwindow* Window;
        RenderingContext* Context;
#endif
    };
}
