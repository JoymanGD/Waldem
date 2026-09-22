#include "wdpch.h"
#include "ProjectManager.h"

#include <fstream>
#include <sstream>
#include <vector>
#include "Waldem/Utils/FileUtils.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "Waldem/Config/EngineConfig.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "Waldem/Scripting/ScriptEngine.h"
#include "Waldem/Utils/JsonUtils.h"

namespace Waldem
{
    namespace
    {
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

        bool RunProcessBlocking(const std::wstring& commandLine, const Path& workingDirectory)
        {
            std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
            mutableCommandLine.push_back(L'\0');

            STARTUPINFOW startupInfo = {};
            startupInfo.cb = sizeof(startupInfo);
            PROCESS_INFORMATION processInfo = {};

            if(!CreateProcessW(
                nullptr,
                mutableCommandLine.data(),
                nullptr,
                nullptr,
                FALSE,
                0,
                nullptr,
                workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
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
    }
    
    void ProjectManager::CreateProject(WString name, Path path)
    {
        //create project folder
        Path projectFolder = path / name.GetData();
        create_directories(projectFolder);

        //create project file
        Path projectFile = projectFolder / name.GetData();
        projectFile.replace_extension(".wproject");
        
        //create content folder
        Path contentFolder = projectFolder / "Content";
        create_directories(contentFolder);

        //create scenes folder
        Path scenesFolder = contentFolder / "Scenes";
        create_directories(scenesFolder);

        //create scripts folder
        Path scriptsFolder = contentFolder / "Scripts";
        create_directories(scriptsFolder);

        //create default scene
        const char* defaultScene = R"([
          {
            "scene_id": 0,
            "parent_scene_id": -1,
            "entity": {
              "name": "Sun",
              "components": {
                "SceneEntity": {
                  "ParentId": 0,
                  "HierarchyDepth": 0,
                  "HierarchySlot": 1,
                  "VisibleInHierarchy": true
                },
                "Waldem.Transform": {
                  "Position": {
                    "x": 53.7033996582,
                    "y": 21.5136680603,
                    "z": -22.0206680298
                  },
                  "Rotation": {
                    "x": 32.0072174072,
                    "y": 0,
                    "z": 0
                  },
                  "LocalScale": {
                    "x": 1,
                    "y": 1,
                    "z": 1
                  }
                },
                "Waldem.Light": {
                  "Color": {
                    "x": 1,
                    "y": 1,
                    "z": 1
                  },
                  "Intensity": 60,
                  "Radius": 10,
                  "InnerCone": 1,
                  "OuterCone": 45,
                  "Softness": 0.001,
                  "AreaWidth": 1,
                  "AreaHeight": 1,
                  "AreaTwoSided": 0,
                  "Type": "Directional"
                }
              }
            }
          },
          {
            "scene_id": 1,
            "parent_scene_id": -1,
            "entity": {
              "name": "Camera",
              "components": {
                "SceneEntity": {
                  "ParentId": 0,
                  "HierarchyDepth": 0,
                  "HierarchySlot": 0,
                  "VisibleInHierarchy": true
                },
                "Waldem.Transform": {
                  "Position": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                  },
                  "Rotation": {
                    "x": 0,
                    "y": 0,
                    "z": 0
                  },
                  "LocalScale": {
                    "x": 1,
                    "y": 1,
                    "z": 1
                  }
                },
                "Waldem.Camera": {
                  "FieldOfView": 60,
                  "AspectRatio": 1.7777778,
                  "NearPlane": 0.001,
                  "FarPlane": 1000
                }
              }
            }
          }
        ])";

        Path defaultSceneFile = scenesFolder / "Default.scene";
        std::ofstream sceneFile(defaultSceneFile);

        if (!sceneFile)
        {
            WD_CORE_ERROR("Couldn't create default scene");
            return;
        }

        sceneFile << defaultScene;
        
        ProjectData newProject = {};
        newProject.Name = name;
        newProject.ProjectFilePath = projectFile;
        newProject.ProjectPath = projectFolder;
        
        rapidjson::Document projectDoc;
        projectDoc.SetObject();

        auto& allocator = projectDoc.GetAllocator();
        projectDoc.AddMember("name", rapidjson::Value(newProject.Name.GetData(), allocator), allocator);
        projectDoc.AddMember("startupscene", rapidjson::Value(newProject.StartupScene.string().c_str(), allocator), allocator);
        rapidjson::StringBuffer buffer;
        rapidjson::Writer writer(buffer);
        projectDoc.Accept(writer);
        std::ofstream file(projectFile);

        if (!file)
        {
            WD_CORE_ERROR("Couldn't create a project file");
            return;
        }
        
        file << buffer.GetString();

        CurrentProject = newProject;
        GenerateProjectFiles();

        //Update engine config
        EngineConfig::SetLastProjectPath(projectFile);
    }

    bool ProjectManager::LoadProject(Path path)
    {
        rapidjson::Document doc;

        if(!JsonUtils::LoadJson(path, doc))
        {
            return false;
        }

        if(!doc.IsObject() || !doc.HasMember("name") || !doc["name"].IsString() || !doc.HasMember("startupscene") || !doc["startupscene"].IsString())
        {
            WD_CORE_ERROR("Project file is missing required fields");
            return false;
        }

        ProjectData project;

        project.Name = doc["name"].GetString();
        project.ProjectFilePath = std::filesystem::absolute(path).lexically_normal();
        project.ProjectPath = project.ProjectFilePath.parent_path();
        project.StartupScene = doc["startupscene"].GetString();

        CurrentProject = project;
        GenerateProjectFiles();

        //Update engine config
        EngineConfig::SetLastProjectPath(project.ProjectFilePath);

        //Reload scripts
        ScriptEngine::ReloadScripts(true);
        
        //Load Scene
        SceneManager::LoadScene(CurrentProject.GetStartupScenePath());

        return true;
    }

    bool ProjectManager::LoadLastProject()
    {
        auto lastProjectPath = GetLastRecentProject();

        if(lastProjectPath.empty())
        {
            return false;
        }

        return LoadProject(lastProjectPath);        
    }

    bool ProjectManager::GenerateProjectFiles()
    {
        const Path engineRoot = FindEngineRoot();
        if(engineRoot.empty())
        {
            WD_CORE_ERROR("Failed to locate engine root for Premake generation");
            return false;
        }

        const Path premakePath = engineRoot / "Vendor" / "premake" / "premake5.exe";
        if(!exists(premakePath))
        {
            WD_CORE_ERROR("Premake executable was not found");
            return false;
        }

        std::wstring commandLine = L"\"" + premakePath.wstring() + L"\" vs2022";
        if(HasProject())
        {
            commandLine += L" --gameproject=\"";
            commandLine += std::filesystem::absolute(CurrentProject.ProjectPath).wstring();
            commandLine += L"\"";
        }

        if(!RunProcessBlocking(commandLine, engineRoot))
        {
            WD_CORE_ERROR("Premake generation failed");
            return false;
        }

        return true;
    }

    Path ProjectManager::GetLastRecentProject()
    {
        return EngineConfig::Get()->LastProjectPath;
    }
}
