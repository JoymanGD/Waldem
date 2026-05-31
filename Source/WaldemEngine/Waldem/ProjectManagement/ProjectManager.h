#pragma once
#include "Waldem/Types/String.h"

namespace Waldem
{
    struct ProjectData
    {
        WString Name = "NewProject";
        Path StartupScene = "Scenes/Default.scene";
    };
    
    class WALDEM_API ProjectManager
    {
    public:
        static void CreateProject(WString name, Path path);
    };
}
