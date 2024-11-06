#pragma once

namespace Waldem
{
    inline std::string GetCurrentFolder()
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        auto currentPath = std::string(buffer);
        size_t pos = currentPath.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            currentPath = currentPath.substr(0, pos);
        }

        return currentPath;
    }
}