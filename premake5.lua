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
IncludeDir["Glad"] = "Waldem/vendor/Glad/include"
IncludeDir["ImGui"] = "Waldem/vendor/imgui"

include "Waldem/vendor/GLFW"
include "Waldem/vendor/Glad"
include "Waldem/vendor/imgui"

project "Waldem"
    location "Waldem"    
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"

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
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}"
    }
    
    links
    {
        "GLFW",
        "Glad",
        "ImGui",
        "opengl32.lib"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "WD_PLATFORM_WINDOWS",
            "WD_BUILD_DLL",
            "GLFW_INCLUDE_NONE"
        }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox")
        }

    filter "configurations:Debug"
        defines "WD_DEBUG"
        staticruntime "off"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "WD_RELEASE"
        staticruntime "off"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "WD_DIST"
        staticruntime "off"
        runtime "Release"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

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
        staticruntime "off"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "WD_RELEASE"
        staticruntime "off"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "WD_DIST"
        staticruntime "off"
        runtime "Release"
        optimize "On"