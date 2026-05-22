project "ImGuizmo"
    location "."
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("../../Build/%{cfg.buildcfg}/%{prj.name}")
    objdir ("../../Intermediate/%{cfg.buildcfg}/%{prj.name}")

    files
    {
        "ImGuizmo.cpp",
        "ImGuizmo.h"
    }

    includedirs
    {
        ".",
        "../imgui"
    }

    links
    {
        "imgui"
    }

    defines
    {
        "IMGUIZMO_EXPORTS",
        "IMGUI_API=__declspec(dllimport)"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter {}
