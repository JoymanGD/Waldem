#include "wdpch.h"
#include "Main.h"

#include "EditorLayer.h"
#include "Waldem/Engine.h"
#include "Waldem/Layers/DebugLayer.h"
#include "Waldem/Log/Log.h"

#ifdef WD_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Waldem::Log::Init();

    auto engine = new Waldem::Engine();
    
    auto editorLayer = new Waldem::EditorLayer(engine->GetWindow());
    editorLayer->Initialize();
    engine->PushOverlay(editorLayer);
    
    engine->Run();
    delete engine;
}

#endif