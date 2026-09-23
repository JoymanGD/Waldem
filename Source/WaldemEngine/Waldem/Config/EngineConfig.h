#pragma once
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "Waldem/Utils/JsonUtils.h"

namespace Waldem
{
    struct EngineConfig
    {
    private:
        static inline EngineConfig* Instance;
        
    public:
        WString LastProjectPath;
        WArray<WString> RecentProjects;
        
        static EngineConfig* Get()
        {
            if(!Instance)
            {
                Load();
            }

            return Instance;
        }

        static void Load()
        {
            rapidjson::Document doc;

            Path configPath = Path(ENGINE_PATH) / "Engine.json";

            if(!JsonUtils::LoadJson(configPath, doc))
            {
                doc.SetObject();
                auto& allocator = doc.GetAllocator();
                doc.AddMember("LastProjectPath", rapidjson::Value(), allocator);
                doc.AddMember("RecentProjects", rapidjson::Value(rapidjson::kArrayType), allocator);
                std::ofstream output(configPath);

                rapidjson::OStreamWrapper outputWrapper(output);
                rapidjson::PrettyWriter writer(outputWrapper);

                doc.Accept(writer);
            }

            Instance = new EngineConfig;

            if(doc.HasMember("LastProjectPath") && doc["LastProjectPath"].IsString())
            {
                Instance->LastProjectPath = doc["LastProjectPath"].GetString();
            }
            
            if(doc.HasMember("RecentProjects") && doc["RecentProjects"].IsArray())
            {
                auto docArray = doc["RecentProjects"].GetArray();

                for (auto& entry : docArray)
                {
                    Instance->RecentProjects.Add(entry.GetString());
                }
            }
        }

        static void Write()
        {
            auto instance = Get();
            rapidjson::Document doc;

            Path configPath = Path(ENGINE_PATH) / "Engine.json";
            
            if(!JsonUtils::LoadJson(configPath, doc))
            {
                WD_CORE_ERROR("Failed to load Engine config");
                return;
            }

            auto& allocator = doc.GetAllocator();

            if(doc.HasMember("LastProjectPath"))
            {
                doc["LastProjectPath"].SetString(instance->LastProjectPath.C_Str(), instance->LastProjectPath.Length(), allocator);
            }

            if(doc.HasMember("RecentProjects"))
            {
                rapidjson::Value& projects = doc["RecentProjects"];
                projects.Clear();
                for (auto& project : instance->RecentProjects)
                {
                    rapidjson::Value value;
                    value.SetString(project.C_Str(), project.Length(), doc.GetAllocator());
                    projects.PushBack(value, doc.GetAllocator());
                }
            }
            
            std::ofstream output(configPath);

            if (!output.is_open())
                return;

            rapidjson::OStreamWrapper outputWrapper(output);
            rapidjson::PrettyWriter writer(outputWrapper);

            doc.Accept(writer);
        }
        
        static void SetLastProjectPath(WString& lastProjectPath)
        {
            AddRecentProject(lastProjectPath);
            Get()->LastProjectPath = lastProjectPath;
            Write();
        }

        static void AddRecentProject(WString& project)
        {
            EngineConfig* instance = Get();
            if(!instance->RecentProjects.Contains(project))
            {
                instance->RecentProjects.Add(project);
            }
        }
    };
}
