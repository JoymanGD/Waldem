#pragma once
#include <flecs.h>

#include "Waldem/Types/FreeList.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    enum IdType
    {
        LightIdType,
        DrawIdType,
        RTXInstanceIdType
    };
    
    class WALDEM_API IdManager
    {
    private:
        inline static WMap<flecs::entity, WMap<IdType, int>> EntityIdMap;
        inline static FreeList LightIdFreeList;
        inline static FreeList DrawIdFreeList;
        inline static FreeList RTXInstanceIdFreeList;
        
    public:
        static int AddId(flecs::entity entity, IdType idType)
        {
            int id = -1;

            switch (idType)
            {
            case LightIdType:
                {
                    id = LightIdFreeList.Allocate();
                    break;
                }
            case DrawIdType:
                {
                    id = DrawIdFreeList.Allocate();
                    break;
                }
            case RTXInstanceIdType:
                {
                    id = RTXInstanceIdFreeList.Allocate();
                    break;
                }
            }
            
            EntityIdMap[entity][idType] = id;

            return id;
        }

        static bool GetId(flecs::entity entity, IdType idType, int& id)
        {
            if(EntityIdMap.Contains(entity))
            {
                if(EntityIdMap[entity].Contains(idType))
                {
                    id = EntityIdMap[entity][idType];
                    return true;
                }
                
                return false;
            }
            
            return false;
        }

        static void RemoveId(flecs::entity entity, IdType idType)
        {
            if(EntityIdMap.Contains(entity))
            {
                int id = -1;
                
                if(EntityIdMap[entity].Contains(idType))
                {
                    id = EntityIdMap[entity][idType];
                    EntityIdMap[entity].Remove(idType);
                }

                if(EntityIdMap[entity].IsEmpty())
                {
                    EntityIdMap.Remove(entity);
                }

                if(id >= 0)
                {
                    switch (idType)
                    {
                    case LightIdType:
                        {
                            LightIdFreeList.Free(id);
                            break;
                        }
                    case DrawIdType:
                        {
                            DrawIdFreeList.Free(id);
                            break;
                        }
                    case RTXInstanceIdType:
                        {
                            RTXInstanceIdFreeList.Free(id);
                            break;
                        }
                    }
                }
            }
        }

        static bool GetEntityById(int id, const IdType idType, flecs::entity& outEntity)
        {
            for (const auto& pair : EntityIdMap)
            {
                const flecs::entity& entity = pair.key;
                const WMap<IdType, int>& ids = pair.value;

                if (ids.Contains(idType) && ids[idType] == id)
                {
                    outEntity = entity;
                    return true;
                }
            }

            return false;
        }

        static void Reset()
        {
            EntityIdMap.Clear();
            LightIdFreeList.Clear();
            DrawIdFreeList.Clear();
            RTXInstanceIdFreeList.Clear();
        }
    };
}
