#pragma once
#include "Waldem/Serialization/Asset.h"

namespace Waldem
{
    class IImporter
    {
    public:
        virtual ~IImporter() = default;
        
        virtual WArray<Asset*> Import(const Path& from, Path& to, bool relative = true) = 0;
    };
}
