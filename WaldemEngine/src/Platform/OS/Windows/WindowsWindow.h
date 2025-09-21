#pragma once

#include "Waldem/Window.h"
#include <SDL.h>

namespace Waldem
{
    struct WindowData
    {
        WString Title;
        Point2 Position;
        float Width, Height;
        bool VSync;
        uint Id;

        EventCallbackFn EventCallback;
    };
    
    class WindowsWindow : public CWindow
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void Begin() override;
        void End() override;
    
        Vector2 GetPosition() const override { return Data.Position; }
        float GetWidth() const override { return Data.Width; }
        float GetHeight() const override { return Data.Height; }
    
        void SetEventCallback(const EventCallbackFn& callback) override { Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        void SetTitle(WString title) override;
        bool IsVSync() const override { return Data.VSync; }

        HWND GetWindowsHandle() const override;
        
        SDL_Window* NativeWindow;

    private:
        void Init(const WindowProps& props);
        void Shutdown();
    
        void RequestEvents();
        void ProcessEvents();

        WindowData Data;
        WArray<SDL_Event*> Events;
    };
}
