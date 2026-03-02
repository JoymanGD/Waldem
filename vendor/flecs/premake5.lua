project "flecs"
    location "."
    kind "StaticLib"
    language "C"
    staticruntime "off"

    targetdir ("../../Build/%{cfg.buildcfg}/%{prj.name}")
    objdir ("../../Intermediate/%{cfg.buildcfg}/%{prj.name}")

    files
    {
        "flecs.c"
    }

    includedirs
    {
        "include"
    }

    defines
    {
        "flecs_STATIC",
        "WIN32_LEAN_AND_MEAN"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter {}