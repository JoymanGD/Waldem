#pragma once

#include <Waldem.h>
#include "Scenes/DefaultScene.h"
#include "Scenes/PhysicsTestScene.h"

namespace Sandbox
{
	class SandboxApp : public Waldem::Application
	{
	public:
		SandboxApp()
		{
			OpenScene(new DefaultScene);
			// OpenScene(new PhysicsTestScene);
		}

		~SandboxApp()
		{

		}
	};
}
