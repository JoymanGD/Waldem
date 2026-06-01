#pragma once
#include "Waldem/Types/String.h"

namespace Waldem
{
    #define PROJECT_CONTENT_PATH ProjectManager::CurrentProject.GetContentPath()
    struct ProjectData
    {
        WString Name = "NewProject";
        Path ProjectFilePath = "";
        Path ProjectPath = "";
        Path StartupScene = "Scenes/Default.scene";

        Path GetContentPath() const
        {
            return ProjectPath / "Content";
        }

        Path GetStartupScenePath() const
        {
            return GetContentPath() / StartupScene;
        }

        Path GetScriptsPath() const
        {
            return GetContentPath() / "Scripts";
        }

        bool IsValid() const
        {
            return !ProjectPath.empty() && !Name.IsEmpty();
        }
    };
    
    class WALDEM_API ProjectManager
    {
    public:
        static void CreateProject(WString name, Path path);
        static bool LoadProject(Path path);
        static bool HasProject() { return CurrentProject.IsValid(); }
        static inline ProjectData CurrentProject;
    };
}
