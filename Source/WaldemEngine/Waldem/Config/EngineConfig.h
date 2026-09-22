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
        Path LastProjectPath;
        
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
        }

        static void Write()
        {
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
                doc["LastProjectPath"].SetString(Instance->LastProjectPath.string().c_str(), static_cast<rapidjson::SizeType>(Instance->LastProjectPath.string().size()), allocator);
            }
            
            std::ofstream output(configPath);

            if (!output.is_open())
                return;

            rapidjson::OStreamWrapper outputWrapper(output);
            rapidjson::PrettyWriter writer(outputWrapper);

            doc.Accept(writer);
        }
        
        static void SetLastProjectPath(Path& lastProjectPath)
        {
            Get()->LastProjectPath = lastProjectPath;
            Write();
        }
    };
}
