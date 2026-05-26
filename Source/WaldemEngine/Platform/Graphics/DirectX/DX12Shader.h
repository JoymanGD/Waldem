#pragma once
#include <d3d12.h>
#include <filesystem>
#include <vector>
#include "Waldem/Renderer/GraphicResource.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
#define MAX_CBV_PER_ROOT_SIGNATURE 14
#define MAX_SRV_PER_ROOT_SIGNATURE 128
#define MAX_UAV_PER_ROOT_SIGNATURE 64
#define MAX_SAMPLER_PER_ROOT_SIGNATURE 14

    inline Path ResolveProjectRootFromRuntime()
    {
        Path current = GetCurrentFolder();
        for(int i = 0; i < 6 && !current.empty(); ++i)
        {
            if(std::filesystem::exists(current / "Source") && std::filesystem::exists(current / "Build"))
            {
                return current;
            }

            const Path parent = current.parent_path();
            if(parent == current)
            {
                break;
            }

            current = parent;
        }

        return {};
    }

    inline std::vector<Path> GetShaderSearchRoots()
    {
        std::vector<Path> roots;
        const Path projectRoot = ResolveProjectRootFromRuntime();
        if(!projectRoot.empty())
        {
            const Path engineShaders = projectRoot / "Source" / "WaldemEngine" / "Shaders";
            if(std::filesystem::exists(engineShaders))
            {
                roots.push_back(engineShaders);
            }

            const Path editorShaders = projectRoot / "Source" / "WaldemEditor" / "Shaders";
            if(std::filesystem::exists(editorShaders))
            {
                roots.push_back(editorShaders);
            }

            const Path sandboxShaders = projectRoot / "Source" / "Sandbox" / "Shaders";
            if(std::filesystem::exists(sandboxShaders))
            {
                roots.push_back(sandboxShaders);
            }
        }

        const Path runtimeShaders = GetCurrentFolder() / "Shaders";
        if(std::filesystem::exists(runtimeShaders))
        {
            roots.push_back(runtimeShaders);
        }

        return roots;
    }

    inline Path ResolveShaderFilePath(const Path& shaderName, const wchar_t* extension)
    {
        const std::wstring shaderFileName = shaderName.filename().wstring() + extension;
        const Path shaderFolder = shaderName.parent_path();

        for(const Path& root : GetShaderSearchRoots())
        {
            const Path candidate = root / shaderFolder / shaderFileName;
            if(std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }

        const auto roots = GetShaderSearchRoots();
        if(!roots.empty())
        {
            return roots.front() / shaderFolder / shaderFileName;
        }

        return GetCurrentFolder() / "Shaders" / shaderFolder / shaderFileName;
    }
}
