#pragma once
#include "Waldem/Types/String.h"

namespace Waldem
{
    #define PROJECT_CONTENT_PATH ProjectManager::CurrentProject.GetContentPath()
    struct ProjectData
    {
        WString Name = "NewProject";
        Path ProjectPath = "C:/WaldemGames/NewProject";
        Path StartupScene = "Scenes/Default.scene";

        Path GetContentPath()
        {
            Path contenPath = ProjectPath / "Content";
            return contenPath;
        }
    };
    
    class WALDEM_API ProjectManager
    {
    public:
        static void CreateProject(WString name, Path path);
        static bool LoadProject(Path path);
        static inline ProjectData CurrentProject;
    };
}
