#pragma once

#include <Waldem.h>
#include "imgui/imgui.h"
#include "Layers/ExampleLayer.h"
#include "Scenes/DefaultScene.h"

namespace Sandbox
{
	class SandboxApp : public Waldem::Application
	{
	public:
		SandboxApp()
		{
			PushLayer(new ExampleLayer());
			OpenScene(new DefaultScene);
		}

		~SandboxApp()
		{

		}
	};
}