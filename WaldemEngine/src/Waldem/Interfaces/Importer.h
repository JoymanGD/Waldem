#pragma once

#include <filesystem>

namespace Waldem
{
    template<typename T>
    class IImporter
    {
    public:
        virtual ~IImporter() = default;
        
        virtual T* Import(const Path& path, bool relative = true) = 0;
    };
}