#pragma once

#include <Waldem.h>
#include "Scenes\RenderingTestScene.h"
#include "Scenes/PhysicsTestScene.h"

namespace Sandbox
{
	class SandboxApp : public Waldem::Application
	{
	public:
		SandboxApp()
		{
			// OpenScene(new RenderingTestScene);
			InitializeLayers();
			// OpenScene(new PhysicsTestScene);
		}

		~SandboxApp()
		{

		}
	};
}
