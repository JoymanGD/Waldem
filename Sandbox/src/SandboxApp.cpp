#include <wdpch.h>
#include "SandboxApp.h"
#include "Waldem/Application.h"

Waldem::Application* Waldem::CreateApplication()
{
    return new Sandbox::SandboxApp();
}