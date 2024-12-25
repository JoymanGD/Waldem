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
IncludeDir["ImGui"] = "Waldem/vendor/imgui"
IncludeDir["ImGuizmo"] = "Waldem/vendor/ImGuizmo"
IncludeDir["glm"] = "Waldem/vendor/glm"
IncludeDir["SDL"] = "Waldem/vendor/SDL/include"
IncludeDir["ECS"] = "Waldem/vendor/ECS/include"
IncludeDir["SPDLog"] = "Waldem/vendor/spdlog/include"
IncludeDir["Assimp"] = "Waldem/vendor/assimp/include"
IncludeDir["stb"] = "Waldem/vendor/stb/include"

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
        "%{prj.name}/vendor/imgui/**.h",
        "%{prj.name}/vendor/imgui/**.cpp",
        "%{prj.name}/vendor/imgui/backends/**.h",
        "%{prj.name}/vendor/imgui/backends/**.cpp",
        "%{prj.name}/vendor/ImGuizmo/**.h",
        "%{prj.name}/vendor/ImGuizmo/**.cpp",
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl"
    }
    
    includedirs
    {
        "%{prj.name}/src",
        "%{IncludeDir.SPDLog}",
        "%{IncludeDir.Assimp}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.SDL}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.ECS}"
    }
    
    libdirs
    {
        "%{prj.name}/vendor/assimp/lib",
        "%{prj.name}/vendor/SDL/lib"
    }

    links
    {
        "SDL2", "SDL2main",
        "opengl32.lib",
        "assimp-vc142-mt.lib",
        "d3d12", "dxgi", "d3dcompiler"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "WD_PLATFORM_WINDOWS",
            "WD_BUILD_DLL"
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
        
    filter { "files:**.hlsl" }
       flags { "ExcludeFromBuild", 'NoPCH' }
       shadermodel "5.1"
    filter { "files:**.ps.hlsl" }
       removeflags "ExcludeFromBuild"
       shadertype "Pixel"
       shaderentry "main"
    filter { "files:**.vs.hlsl" }
       removeflags "ExcludeFromBuild"
       shadertype "Vertex"
       shaderentry "main"
    filter {}

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"
    dependson { "Waldem" }

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.glsl",
        "%{prj.name}/src/**.hlsl"
    }

    links
    {
        "Waldem"
    }

    includedirs
    {
        "Waldem/src",
        "%{IncludeDir.SPDLog}",
        "%{IncludeDir.Assimp}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.SDL}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.ECS}"
    }

    filter "system:windows"
        systemversion "latest"
        defines "WD_PLATFORM_WINDOWS"
        
        postbuildcommands
        {
            "if exist %{wks.location}bin\\Debug\\Sandbox\\Shaders (rmdir /s /q %{wks.location}bin\\Debug\\Sandbox\\Shaders)",
            "if exist %{wks.location}bin\\Debug\\Sandbox\\Content (rmdir /s /q %{wks.location}bin\\Debug\\Sandbox\\Content)",
            "echo Copying files...",
            "{COPY} %{wks.location}%{prj.name}\\src\\Shaders\\*.hlsl %{cfg.targetdir}\\Shaders\\",
            "{COPY} %{wks.location}Waldem\\src\\Shaders\\*.hlsl %{cfg.targetdir}\\Shaders\\",
            "{COPY} %{wks.location}Waldem\\vendor\\SDL\\lib\\SDL2.dll %{cfg.targetdir}\\",
            "{COPY} %{wks.location}Waldem\\vendor\\assimp\\lib\\assimp-vc142-mt.dll %{cfg.targetdir}\\",
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
        
    filter { "files:**.hlsl" }
       flags { "ExcludeFromBuild", 'NoPCH' }
       shadermodel "5.1"
    filter { "files:**.ps.hlsl" }
       removeflags "ExcludeFromBuild"
       shadertype "Pixel"
       shaderentry "main"
    filter { "files:**.vs.hlsl" }
       removeflags "ExcludeFromBuild"
       shadertype "Vertex"
       shaderentry "main"
    filter {}