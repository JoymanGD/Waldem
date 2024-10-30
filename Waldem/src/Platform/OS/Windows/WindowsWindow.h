#pragma once

#include "Waldem/Window.h"
#include <SDL.h>

namespace Waldem
{
    struct WindowData
    {
        std::string Title;
        float Width, Height;
        bool VSync;

        EventCallbackFn EventCallback;
    };
    
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

        WindowData Data;
        SDL_Window* Window;
    };
}
