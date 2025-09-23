#pragma once
#include "Waldem/Serialization/Asset.h"

namespace Waldem
{
    class IImporter
    {
    public:
        virtual ~IImporter() = default;
        
        virtual WArray<Asset*> ImportTo(const Path& from, const Path& to, bool relative = true) = 0;
    };
}