#pragma once

namespace Waldem
{
    template<typename T>
    class IImporter
    {
    public:
        virtual ~IImporter() = default;
        
        virtual T* Import(std::string& path, bool relative = true) = 0;
    };
}