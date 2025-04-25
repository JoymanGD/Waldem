#pragma once

#include "Core.h"
#include "Events/Event.h"

namespace Waldem
{
	using EventCallbackFn = std::function<void(Event&)>;
	
	struct WindowProps
	{
		WString Title;
        Point2 Position;
		float Width;
		float Height;

		WindowProps(const WString& title = "Waldem Engine",
					float width = 1920,
					float height = 1080,
					Point2 position = { 50, 50 })
			: Title(title), Width(width), Height(height), Position(position) {}
	};
	
	class WALDEM_API CWindow
	{
	public:

		virtual ~CWindow() {}

		virtual void OnUpdate() = 0;

		virtual float GetWidth() const = 0;
		virtual float GetHeight() const = 0;
		virtual Vector2 GetPosition() const = 0;
		virtual std::array<float, 2> GetSize() const { return { GetWidth(), GetHeight() }; }

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual void SetTitle(WString title) = 0;
		virtual bool IsVSync() const = 0;

		static void* GetNativeWindow();
		virtual HWND GetWindowsHandle() const = 0;

		static CWindow* Create(const WindowProps& props = WindowProps());

        inline static CWindow* Instance;
	};
}
