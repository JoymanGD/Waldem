workspace "Waldem"
    architecture "x64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Waldem/vendor/GLFW/include"

include "Waldem/vendor/GLFW"

project "Waldem"
    location "Waldem"    
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    pchheader "wdpch.h"
    pchsource "Waldem/src/wdpch.cpp"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/vendor/spdlog/include",
        "%{IncludeDir.GLFW}"
    }
    
    links
    {
        "GLFW",
        "opengl32.lib"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "WD_PLATFORM_WINDOWS",
            "WD_BUILD_DLL"
        }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
        }

    filter "configurations:Debug"
        defines "WD_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "WD_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "WD_DIST"
        runtime "Release"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs
    {
        "Waldem/vendor/spdlog/include",
        "Waldem/src",
        "Waldem/vendor"
    }

    links
    {
        "Waldem"
    }

    filter "system:windows"
        systemversion "latest"
        defines "WD_PLATFORM_WINDOWS"

    filter "configurations:Debug"
        defines "WD_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "WD_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "WD_DIST"
        runtime "Release"
        optimize "On"