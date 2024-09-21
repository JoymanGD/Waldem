workspace "Waldem"
    architecture "x64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}"

IncludeDir = {}
IncludeDir["GLFW"] = "Waldem/vendor/GLFW/include"
IncludeDir["Glad"] = "Waldem/vendor/Glad/include"
IncludeDir["ImGui"] = "Waldem/vendor/imgui"
IncludeDir["glm"] = "Waldem/vendor/glm"

include "Waldem/vendor/GLFW"
include "Waldem/vendor/Glad"
include "Waldem/vendor/imgui"

project "Waldem"
    location "Waldem"    
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    pchheader "wdpch.h"
    pchsource "Waldem/src/wdpch.cpp"
    
    defines
    {
        "_CRT_SECURE_NO_WARNINGS"
    }

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl"
    }
    
    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/vendor/spdlog/include",
        "%{prj.name}/vendor/assimp/include",
        "%{prj.name}/vendor/stb/include",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}"
    }
    
    libdirs
    {
        "%{prj.name}/vendor/assimp/lib"
    }

    links
    {
        "GLFW",
        "Glad",
        "ImGui",
        "opengl32.lib",
        "assimp-vc142-mt.lib"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "WD_PLATFORM_WINDOWS",
            "WD_BUILD_DLL",
            "GLFW_INCLUDE_NONE"
        }

    filter "configurations:Debug"
        defines "WD_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "WD_RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "WD_DIST"
        runtime "Release"
        optimize "on"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.glsl"
    }

    includedirs
    {
        "Waldem/vendor/assimp/include",
        "Waldem/vendor/spdlog/include",
        "Waldem/vendor/stb/include",
        "Waldem/src",
        "Waldem/vendor",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.glm}"
    }

    links
    {
        "Waldem"
    }

    filter "system:windows"
        systemversion "latest"
        defines "WD_PLATFORM_WINDOWS"
        
        postbuildcommands
        {
            "if exist %{wks.location}bin\\Debug\\Sandbox\\Shaders (rmdir /s /q %{wks.location}bin\\Debug\\Sandbox\\Shaders)",
            "if exist %{wks.location}bin\\Debug\\Sandbox\\Content (rmdir /s /q %{wks.location}bin\\Debug\\Sandbox\\Content)",
            "echo Copying files...",
            "{COPY} %{wks.location}%{prj.name}\\src\\Shaders\\Test\\*.glsl %{cfg.targetdir}\\Shaders\\",
            "{COPYDIR} %{wks.location}%{prj.name}\\Content\\ %{cfg.targetdir}\\Content\\"
        }

    filter "configurations:Debug"
        defines "WD_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "WD_RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "WD_DIST"
        runtime "Release"
        optimize "on"