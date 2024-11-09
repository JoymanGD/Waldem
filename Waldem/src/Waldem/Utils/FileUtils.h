#pragma once

namespace Waldem
{
    inline String GetCurrentFolder()
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        auto currentPath = String(buffer);
        size_t pos = currentPath.find_last_of("\\/");
        if (pos != String::npos)
        {
            currentPath = currentPath.substr(0, pos);
        }

        return currentPath;
    }
}