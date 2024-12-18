#pragma once

#include <Waldem.h>
#include "Scenes/DefaultScene.h"

namespace Sandbox
{
	class SandboxApp : public Waldem::Application
	{
	public:
		SandboxApp()
		{
			OpenScene(new DefaultScene);
		}

		~SandboxApp()
		{

		}
	};
}