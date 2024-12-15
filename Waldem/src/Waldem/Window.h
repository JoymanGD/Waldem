#pragma once

#include "Core.h"
#include "Events/Event.h"

namespace Waldem
{
	using EventCallbackFn = std::function<void(Event&)>;
	
	struct WindowProps
	{
		String Title;
		float Width;
		float Height;

		WindowProps(const String& title = "Waldem Engine",
					float width = 1280,
					float height = 720)
			: Title(title), Width(width), Height(height) {}
	};
	
	class WALDEM_API Window
	{
	public:

		virtual ~Window() {}

		virtual void OnUpdate() = 0;

		virtual float GetWidth() const = 0;
		virtual float GetHeight() const = 0;
		virtual std::array<float, 2> GetSize() const { return { GetWidth(), GetHeight() }; }

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual void SetTitle(String title) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;
		virtual HWND GetWindowsHandle() const = 0;

		static Window* Create(const WindowProps& props = WindowProps());
	};
}
