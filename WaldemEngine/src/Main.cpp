#include "wdpch.h"
#include "Main.h"

#include "Waldem\Engine.h"
#include "Waldem/Log/Log.h"

#ifdef WD_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Waldem::Log::Init();

    auto app = new Waldem::Engine();
    app->Run();
    delete app;
}

#endif