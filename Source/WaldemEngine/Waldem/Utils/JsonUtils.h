#pragma once
#include <fstream>
#include <rapidjson/document.h>

namespace Waldem
{
    class WALDEM_API JsonUtils
    {
    public:
        static bool LoadJson(Path& path, rapidjson::Document& document)
        {
            std::ifstream file(path);

            if (!file)
            {
                WD_CORE_ERROR("Failed to open json file");
                return false;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            std::string json = buffer.str();

            document.Parse(json.c_str());

            if (document.HasParseError())
            {
                WD_CORE_ERROR("Failed to parse json file");
                return false;
            }

            return true;
        }
    };
}
