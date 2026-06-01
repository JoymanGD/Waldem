#include "wdpch.h"
#include "ProjectBuilder.h"

#include <fstream>
#include <vector>

#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    namespace
    {
        constexpr const wchar_t* RuntimeExecutableName = L"Sandbox.exe";

        std::string ReadCommandOutput(const std::string& command)
        {
            std::string output;
            FILE* pipe = _popen(command.c_str(), "r");
            if(pipe == nullptr)
            {
                return output;
            }

            char buffer[512];
            while(fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                output += buffer;
            }

            _pclose(pipe);

            while(!output.empty() && (output.back() == '\n' || output.back() == '\r'))
            {
                output.pop_back();
            }

            return output;
        }

        bool RunProcessBlocking(const std::wstring& commandLine, const Path& workingDirectory)
        {
            STARTUPINFOW startupInfo = {};
            startupInfo.cb = sizeof(startupInfo);

            PROCESS_INFORMATION processInfo = {};
            std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
            mutableCommandLine.push_back(L'\0');

            const std::wstring workingDirectoryString = workingDirectory.wstring();
            if(!CreateProcessW(
                nullptr,
                mutableCommandLine.data(),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW,
                nullptr,
                workingDirectoryString.c_str(),
                &startupInfo,
                &processInfo))
            {
                return false;
            }

            WaitForSingleObject(processInfo.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(processInfo.hProcess, &exitCode);

            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);

            return exitCode == 0;
        }

        Path FindEngineRoot()
        {
            Path current = GetCurrentFolder();
            for(int i = 0; i < 8 && !current.empty(); ++i)
            {
                if(exists(current / "Waldem.sln"))
                {
                    return current;
                }

                current = current.parent_path();
            }

            return {};
        }

        bool CopyDirectoryContents(const Path& source, const Path& destination)
        {
            std::error_code errorCode;
            create_directories(destination, errorCode);
            if(errorCode)
            {
                return false;
            }

            for(const auto& entry : std::filesystem::directory_iterator(source, errorCode))
            {
                if(errorCode)
                {
                    return false;
                }

                const Path targetPath = destination / entry.path().filename();
                copy(entry.path(), targetPath, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, errorCode);
                if(errorCode)
                {
                    return false;
                }
            }

            return true;
        }

        const char* ToConfigurationString(ProjectBuildConfiguration configuration)
        {
            return configuration == ProjectBuildConfiguration::Debug ? "Debug" : "Release";
        }
    }

    bool ProjectBuilder::BuildCurrentProject(const Path& outputDirectory, ProjectBuildConfiguration configuration)
    {
        LastBuildOutputPath.clear();

        if(!ProjectManager::HasProject())
        {
            LastBuildStatus = "No project loaded";
            return false;
        }

        if(outputDirectory.empty())
        {
            LastBuildStatus = "No output directory selected";
            return false;
        }

        const Path engineRoot = FindEngineRoot();
        if(engineRoot.empty())
        {
            LastBuildStatus = "Failed to locate engine root";
            return false;
        }

        const Path vsWherePath = Path("C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe");
        if(!exists(vsWherePath))
        {
            LastBuildStatus = "vswhere.exe was not found";
            return false;
        }

        const std::string msbuildPathString = ReadCommandOutput(
            "\"" + vsWherePath.string() + "\" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe");
        if(msbuildPathString.empty())
        {
            LastBuildStatus = "Failed to locate MSBuild";
            return false;
        }

        const char* configurationName = ToConfigurationString(configuration);
        const Path solutionPath = engineRoot / "Waldem.sln";
        const Path projectScriptsPath = ProjectManager::CurrentProject.GetScriptsPath();
        const std::wstring configurationNameW = std::wstring(configurationName, configurationName + strlen(configurationName));
        const Path buildLogPath = std::filesystem::absolute(outputDirectory / (ProjectManager::CurrentProject.Name.ToString() + ".build.log")).lexically_normal();

        std::wstring commandLine =
            L"\"" + Path(msbuildPathString).wstring() + L"\" \"" + solutionPath.wstring() +
            L"\" /t:\"ScriptEngine;Sandbox\" /p:Configuration=" + configurationNameW +
            L" /p:Platform=x64 /p:WaldemProjectScriptsDir=\"" + projectScriptsPath.wstring() +
            L"\" /fl /flp:logfile=\"" + buildLogPath.wstring() + L"\";verbosity=normal /nologo";

        if(!RunProcessBlocking(commandLine, engineRoot))
        {
            LastBuildStatus = "MSBuild failed. See log: " + buildLogPath.string();
            return false;
        }

        const Path runtimeBuildPath = engineRoot / "Build" / configurationName / "Sandbox";
        if(!exists(runtimeBuildPath))
        {
            LastBuildStatus = "Runtime output folder was not produced";
            return false;
        }

        const Path packageRoot = std::filesystem::absolute(outputDirectory / ProjectManager::CurrentProject.Name.ToString()).lexically_normal();

        std::error_code errorCode;
        remove_all(packageRoot, errorCode);
        errorCode.clear();
        create_directories(packageRoot, errorCode);
        if(errorCode)
        {
            LastBuildStatus = "Failed to create package folder";
            return false;
        }

        if(!CopyDirectoryContents(runtimeBuildPath, packageRoot))
        {
            LastBuildStatus = "Failed to copy runtime files";
            return false;
        }

        if(!CopyDirectoryContents(ProjectManager::CurrentProject.GetContentPath(), packageRoot / "Content"))
        {
            LastBuildStatus = "Failed to copy project content";
            return false;
        }

        const Path projectFileTarget = packageRoot / ProjectManager::CurrentProject.ProjectFilePath.filename();
        errorCode.clear();
        copy_file(ProjectManager::CurrentProject.ProjectFilePath, projectFileTarget, std::filesystem::copy_options::overwrite_existing, errorCode);
        if(errorCode)
        {
            LastBuildStatus = "Failed to copy project file";
            return false;
        }

        const Path packagedRuntimePath = packageRoot / RuntimeExecutableName;
        const Path renamedRuntimePath = packageRoot / (ProjectManager::CurrentProject.Name.ToString() + ".exe");
        if(exists(packagedRuntimePath))
        {
            errorCode.clear();
            rename(packagedRuntimePath, renamedRuntimePath, errorCode);
            if(errorCode)
            {
                LastBuildStatus = "Failed to rename packaged executable";
                return false;
            }
        }
        else
        {
            LastBuildStatus = "Sandbox.exe was not produced";
            return false;
        }

        LastBuildStatus = "Build complete: " + packageRoot.string();
        LastBuildOutputPath = packageRoot;
        return true;
    }
}
