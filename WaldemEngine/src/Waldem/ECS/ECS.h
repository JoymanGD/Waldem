#pragma once

#include "flecs.h"
#include "Components/SceneEntity.h"
#include "Systems/CoreSystem.h"
#include "Waldem/Types/FreeList.h"
#include "Waldem/Types/String.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    namespace ECS
    {
        using Entity = flecs::entity;
        
        inline flecs::world World;
        inline Entity UpdatePipeline;
        inline Entity FixedUpdatePipeline;
        inline Entity DrawPipeline;
        inline Entity GUIPipeline;
        inline WMap<WString, Entity> RegisteredComponents;
        inline FreeList HierarchySlots; 

        inline void RunUpdatePipeline(float deltaTime)
        {
            World.run_pipeline(UpdatePipeline, deltaTime);
        }

        inline void RunFixedUpdatePipeline(float fixedDeltaTime)
        {
            World.run_pipeline(FixedUpdatePipeline, fixedDeltaTime);
        }

        inline void RunDrawPipeline(float deltaTime)
        {
            World.run_pipeline(DrawPipeline, deltaTime);
        }

        inline void RunGUIPipeline(float deltaTime)
        {
            World.run_pipeline(GUIPipeline, deltaTime);
        }
        
        inline int GetEntitiesCount() { return World.query<SceneEntity>().count(); }

        inline bool NameExists(const WString& name)
        {
            return World.lookup(name) != Entity::null();
        }

        inline bool NameExists(const std::string& name)
        {
            return World.lookup(name.c_str()) != Entity::null();
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
        
        Entity CreateEntity(const WString& name = "", bool enabled = true);
        Entity CreateSceneEntity(const WString& name, bool enabled = true, bool visibleInHierarchy = true);
        Entity CloneSceneEntity(Entity entity);

        class Core
        {
        public:
            void Initialize();
            void InitializeSystems();
            
        private:
            void RegisterTypes();
            void RegisterComponents();

            WArray<ICoreSystem*> Systems;
        };
    }
}
