#include "wdpch.h"

#include <stdio.h>
#include "Waldem\Engine.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Log/Log.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/ProjectManagement/ProjectManager.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "EditorLayer.h"
#include "Waldem/ECS/ECS.h"

#ifdef WD_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Waldem::Log::Init();
    ecs_os_set_api_defaults();

    if(argc > 1)
    {
        Waldem::ProjectManager::LoadProject(argv[1]);
    }

    auto engine = new Waldem::Engine();
    
    auto editorLayer = new Waldem::EditorLayer(engine->GetWindow());
    editorLayer->Initialize();
    engine->PushOverlay(editorLayer);

    if(Waldem::ProjectManager::HasProject())
    {
        Waldem::Path startupScenePath = Waldem::ProjectManager::CurrentProject.GetStartupScenePath();
        Waldem::SceneManager::LoadScene(startupScenePath);
    }
    
    engine->Run();
    delete engine;
}

#endif
