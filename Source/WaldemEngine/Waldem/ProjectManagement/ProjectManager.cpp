#include "wdpch.h"
#include "ProjectManager.h"

#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>


void Waldem::ProjectManager::CreateProject(WString name, Path path)
{
    //create project folder
    Path projectFolder = path.append(name.GetData());
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
}
