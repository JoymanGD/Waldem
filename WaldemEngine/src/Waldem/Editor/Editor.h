#pragma once
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    class WALDEM_API Editor
    {
    public:
        inline static int HoveredEntityID;

        static void AddEntityID(uint drawID, uint64 entityID)
        {
            DrawIDToEntityID[drawID] = entityID;
        }

        static void RemoveEntityID(uint drawID)
        {
            DrawIDToEntityID.Remove(drawID);
        }

        static uint64 GetEntityID(uint drawID)
        {
            if (DrawIDToEntityID.Contains(drawID))
            {
                return DrawIDToEntityID[drawID];
            }
            return 0;
        }

    private:
        inline static WMap<uint, uint64> DrawIDToEntityID;
    };
}
