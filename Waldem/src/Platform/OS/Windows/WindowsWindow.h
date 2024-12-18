#pragma once

#include "Waldem/Window.h"
#include <SDL.h>

namespace Waldem
{
    struct WindowData
    {
        String Title;
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
        void SetTitle(String title) override;
        bool IsVSync() const override { return Data.VSync; }

        HWND GetWindowsHandle() const override;
        
        SDL_Window* NativeWindow;

    private:
        void Init(const WindowProps& props);
        void Shutdown();
    
        void ProcessEvents();

        WindowData Data;
    };
}
