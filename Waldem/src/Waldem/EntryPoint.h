#pragma once

#ifdef WD_PLATFORM_WINDOWS

extern Waldem::Application* Waldem::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Waldem::CreateApplication();
	app->Run();

	delete app;
}

#endif