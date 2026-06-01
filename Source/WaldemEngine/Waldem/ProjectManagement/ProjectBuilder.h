#pragma once

#include <string>

#include "ProjectManager.h"

namespace Waldem
{
    enum class ProjectBuildConfiguration
    {
        Debug,
        Release
    };

    class WALDEM_API ProjectBuilder
    {
    public:
        static bool BuildCurrentProject(const Path& outputDirectory, ProjectBuildConfiguration configuration = ProjectBuildConfiguration::Release);
        static const char* GetLastBuildStatus() { return LastBuildStatus.c_str(); }
        static const Path& GetLastBuildOutputPath() { return LastBuildOutputPath; }

    private:
        inline static std::string LastBuildStatus = "Idle";
        inline static Path LastBuildOutputPath = "";
    };
}
