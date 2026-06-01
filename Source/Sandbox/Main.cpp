#include "wdpch.h"

#include "Waldem\Engine.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Log/Log.h"
#include "Waldem/Input/KeyCodes.h"
#include "Waldem/ProjectManagement/ProjectManager.h"
#include "Waldem/Editor/EditorSimulation.h"
#include "Waldem/Utils/FileUtils.h"
#include "GameLayer.h"
#include "Waldem/ECS/ECS.h"
#include <shellapi.h>

#ifdef WD_PLATFORM_WINDOWS

namespace
{
    int RunGame(int argc, wchar_t** argv)
    {
        Waldem::Log::Init();
        ecs_os_set_api_defaults();
        Waldem::EditorSimulation::SetState(Waldem::EditorSimulationState::Play);

        if(argc > 1)
        {
            Waldem::ProjectManager::LoadProject(argv[1]);
        }
        else
        {
            std::error_code errorCode;
            for(const auto& entry : std::filesystem::directory_iterator(Waldem::GetCurrentFolder(), errorCode))
            {
                if(errorCode)
                {
                    break;
                }

                if(entry.is_regular_file() && entry.path().extension() == ".wproject")
                {
                    Waldem::ProjectManager::LoadProject(entry.path());
                    break;
                }
            }
        }

        auto engine = new Waldem::Engine();

        auto gameLayer = new Waldem::GameLayer(engine->GetWindow());
        gameLayer->Initialize();
        engine->PushLayer(gameLayer);
    
        engine->Run();
        delete engine;

        return 0;
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    const int result = RunGame(argc, argv);

    if(argv != nullptr)
    {
        LocalFree(argv);
    }

    return result;
}

#endif
