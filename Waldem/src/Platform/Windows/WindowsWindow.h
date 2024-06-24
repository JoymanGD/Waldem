#pragma once

#include <GLFW/glfw3.h>
#include "Waldem/Window.h"

namespace Waldem {

    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        ~WindowsWindow() override;

        void OnUpdate() override;

        float GetWidth() const override { return Data.Width; }
        float GetHeight() const override { return Data.Height; }
        std::array<float, 2> GetSize() const override { return { Data.Width, Data.Height }; }

        // Window attributes
        void SetEventCallback(const EventCallbackFn& callback) override { Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        virtual void* GetNativeWindow() const override { return Window; }
    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();
    
        GLFWwindow* Window;

        struct WindowData
        {
            std::string Title;
            float Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData Data;
    };

}