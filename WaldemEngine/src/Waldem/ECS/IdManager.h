#pragma once
#include "Waldem/Types/FreeList.h"
#include "Waldem/Types/WMap.h"

namespace Waldem
{
    enum IdType
    {
        LightIdType,
        GlobalDrawIdType,
        BackFaceCullingDrawIdType,
        NoCullingDrawIdType,
        RTXInstanceIdType,
        ParticleSystemIdType,
        GizmoDrawIdType
    };
    
    class WALDEM_API IdManager
    {
    private:
        inline static WMap<uint64, WMap<IdType, int>> EntityIdMap;
        inline static FreeList LightIdFreeList;
        inline static FreeList GlobalDrawIdFreeList;
        inline static FreeList BackFaceCullingDrawIdFreeList;
        inline static FreeList NoCullingDrawIdFreeList;
        inline static FreeList RTXInstanceIdFreeList;
        inline static FreeList ParticleSystemIdFreeList;
        inline static FreeList EditorIconDrawIdFreeList;
        
    public:
        static int AddId(flecs::entity entity, IdType idType)
        {
            int id = -1;

            if(EntityIdMap.Contains(entity.id()) && EntityIdMap[entity.id()].Contains(idType))
            {
                WD_CORE_ERROR("Entity already has an ID assigned of type {}", (int)idType);
            }

            switch (idType)
            {
                case LightIdType:
                {
                    id = LightIdFreeList.Allocate();
                    break;
                }
                case GlobalDrawIdType:
                {
                    id = GlobalDrawIdFreeList.Allocate();
                    break;
                }
                case BackFaceCullingDrawIdType:
                {
                    id = BackFaceCullingDrawIdFreeList.Allocate();
                    break;
                }
                case NoCullingDrawIdType:
                {
                    id = NoCullingDrawIdFreeList.Allocate();
                    break;
                }
                case RTXInstanceIdType:
                {
                    id = RTXInstanceIdFreeList.Allocate();
                    break;
                }
                case ParticleSystemIdType:
                {
                    id = ParticleSystemIdFreeList.Allocate();
                    break;
                }
                case GizmoDrawIdType:
                {
                    id = EditorIconDrawIdFreeList.Allocate();
                    break;
                }
            }
            
            EntityIdMap[entity.id()][idType] = id;

            return id;
        }

        static bool GetId(flecs::entity entity, IdType idType, int& id)
        {
            if(EntityIdMap.Contains(entity.id()))
            {
                if(EntityIdMap[entity.id()].Contains(idType))
                {
                    id = EntityIdMap[entity.id()][idType];
                    return true;
                }
                
                return false;
            }
            
            return false;
        }

        static void RemoveId(flecs::entity entity, IdType idType)
        {
            if(EntityIdMap.Contains(entity.id()))
            {
                int id = -1;
                
                if(EntityIdMap[entity.id()].Contains(idType))
                {
                    id = EntityIdMap[entity.id()][idType];
                    EntityIdMap[entity.id()].Remove(idType);
                }

                if(EntityIdMap[entity.id()].IsEmpty())
                {
                    EntityIdMap.Remove(entity.id());
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
                        case GlobalDrawIdType:
                        {
                            GlobalDrawIdFreeList.Free(id);
                            break;
                        }
                        case BackFaceCullingDrawIdType:
                        {
                            BackFaceCullingDrawIdFreeList.Free(id);
                            break;
                        }
                        case NoCullingDrawIdType:
                        {
                            NoCullingDrawIdFreeList.Free(id);
                            break;
                        }
                        case RTXInstanceIdType:
                        {
                            RTXInstanceIdFreeList.Free(id);
                            break;
                        }
                        case ParticleSystemIdType:
                        {
                            ParticleSystemIdFreeList.Free(id);
                            break;
                        }
                        case GizmoDrawIdType:
                        {
                            EditorIconDrawIdFreeList.Free(id);
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
                const uint64& entityId = pair.key;
                const WMap<IdType, int>& ids = pair.value;

                if (ids.Contains(idType) && ids[idType] == id)
                {
                    outEntity = ECS::World.entity(entityId);
                    return true;
                }
            }

            return false;
        }

        static void Reset()
        {
            EntityIdMap.Clear();
            LightIdFreeList.Clear();
            GlobalDrawIdFreeList.Clear();
            BackFaceCullingDrawIdFreeList.Clear();
            NoCullingDrawIdFreeList.Clear();
            RTXInstanceIdFreeList.Clear();
            ParticleSystemIdFreeList.Clear();
            EditorIconDrawIdFreeList.Clear();
        }
    };
}
