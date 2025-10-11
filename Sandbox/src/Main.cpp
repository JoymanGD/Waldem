#include "wdpch.h"
#include "Main.h"

#include "GameLayer.h"
#include "Waldem/Engine.h"
#include "Waldem/Log/Log.h"

#ifdef WD_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Waldem::Log::Init();

    auto engine = new Waldem::Engine();

    auto gameLayer = new Waldem::GameLayer(engine->GetWindow());
    gameLayer->Initialize();
    engine->PushLayer(gameLayer);
    
    engine->Run();
    delete engine;
}

#endif