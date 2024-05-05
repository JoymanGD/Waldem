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

        uint32_t GetWidth() const override { return Data.Width; }
        uint32_t GetHeight() const override { return Data.Height; }
        std::tuple<uint32_t, uint32_t> GetSize() const override { return std::tuple(Data.Width, Data.Height); }

        // Window attributes
        void SetEventCallback(const EventCallbackFn& callback) override { Data.EventCallback = callback; }
        void SetVSync(bool enabled) override;
        bool IsVSync() const override;

        virtual void* GetNativeWindow() const { return Window; }
    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();
    private:
        GLFWwindow* Window;

        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;

            EventCallbackFn EventCallback;
        };

        WindowData Data;
    };

}