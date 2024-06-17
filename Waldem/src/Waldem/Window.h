#pragma once

#include "Core.h"
#include "Events/Event.h"

namespace Waldem
{
	struct WindowProps
	{
		std::string Title;
		float Width;
		float Height;

		WindowProps(const std::string& title = "Waldem Engine",
					float width = 1280,
					float height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};
	
	//Interface representing a desktop system based Window
	class WALDEM_API Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() {}

		virtual void OnUpdate() = 0;

		virtual float GetWidth() const = 0;
		virtual float GetHeight() const = 0;
		virtual std::array<float, 2> GetSize() const { return { GetWidth(), GetHeight() }; }

		//Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Window* Create(const WindowProps& props = WindowProps());
	};
}
