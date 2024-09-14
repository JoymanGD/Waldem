#include "wdpch.h"
#include "ModelImporter.h"

#include <filesystem>

namespace Waldem
{
    const aiScene* ModelImporter::ImportInternal(std::string& path, ModelImportFlags importFlags, bool relative)
    {
        if(relative)
        {
            path = std::filesystem::current_path().string() + "/" + path;
        }

        return AssimpImporter.ReadFile(path.c_str(), (unsigned)importFlags);
    }
}
