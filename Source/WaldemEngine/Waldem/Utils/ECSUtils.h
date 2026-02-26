#pragma once
#include "Waldem/ECS/ECS.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    inline bool NameExists(const WString& name)
        {
            return ECS::World.lookup(name) != ECS::Entity::null();
        }

        inline bool NameExists(const std::string& name)
        {
            return ECS::World.lookup(name.c_str()) != ECS::Entity::null();
        }

        inline void FormatName(WString& name)
        {
            while(NameExists(name))
            {
                if (!name.IsEmpty())
                {
                    std::string tmp = name.ToString();

                    size_t underscorePos = tmp.find_last_of('_');
                    if (underscorePos != std::string::npos && underscorePos + 1 < tmp.size())
                    {
                        std::string suffix = tmp.substr(underscorePos + 1);
                        if (std::all_of(suffix.begin(), suffix.end(), ::isdigit))
                        {
                            int number = std::stoi(suffix);
                            ++number;
                            tmp = tmp.substr(0, underscorePos + 1) + std::to_string(number);
                            name = tmp.c_str();
                            continue;
                        }
                    }

                    name += "_1";
                }
                else
                {
                    name = "NewEntity";
                }
            }
        }

        inline void FormatName(std::string& name)
        {
            while (NameExists(name))
            {
                if (!name.empty())
                {
                    size_t underscorePos = name.find_last_of('_');
                    if (underscorePos != std::string::npos && underscorePos + 1 < name.size())
                    {
                        std::string suffix = name.substr(underscorePos + 1);
                        if (std::all_of(suffix.begin(), suffix.end(), ::isdigit))
                        {
                            int number = std::stoi(suffix);
                            ++number;
                            name = name.substr(0, underscorePos + 1) + std::to_string(number);
                            continue;
                        }
                    }

                    name += "_1";
                }
                else
                {
                    name = "NewEntity";
                }
            }
        }
}
