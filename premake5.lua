workspace "Waldem"
    architecture "x64"
    startproject "WaldemEngine"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}"
local rootDir = os.getcwd()
local contentPath = path.getabsolute(path.join(rootDir, "Content"))

IncludeDir = {}
IncludeDir["ImGui"] = "Vendor/imgui"
IncludeDir["ImGuizmo"] = "Vendor/ImGuizmo"
IncludeDir["glm"] = "Vendor/glm"
IncludeDir["SDL"] = "Vendor/SDL/include"
IncludeDir["ECS"] = "Vendor/ECS/include"
IncludeDir["SPDLog"] = "Vendor/spdlog/include"
IncludeDir["Assimp"] = "Vendor/assimp/include"
IncludeDir["stb"] = "Vendor/stb/include"
IncludeDir["dxc"] = "Vendor/dxc/inc"
IncludeDir["mono"] = "Vendor/mono/include"
IncludeDir["flecs"] = "Vendor/flecs/include"
IncludeDir["rapidjson"] = "Vendor/rapidjson/include"

filter { "files:**.hlsl" }
   flags { "ExcludeFromBuild", 'NoPCH' }
   shadermodel "6.6"
filter { "files:**.ps.hlsl" }
   removeflags "ExcludeFromBuild"
   shadertype "Pixel"
   shaderentry "main"
filter { "files:**.vs.hlsl" }
   removeflags "ExcludeFromBuild"
   shadertype "Vertex"
   shaderentry "main"
filter { "files:**.comp.hlsl" }
   removeflags "ExcludeFromBuild"
   shadertype "Compute"
   shaderentry "main"

project "Vendor"
    location "Vendor"    
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/imgui/**.h",
        "%{prj.name}/imgui/**.cpp",
        "%{prj.name}/imgui/backends/**.h",
        "%{prj.name}/imgui/backends/**.cpp",
        "%{prj.name}/ImGuizmo/**.h",
        "%{prj.name}/ImGuizmo/**.cpp",
        "%{prj.name}/glm/glm/**.hpp",
        "%{prj.name}/glm/glm/**.inl",
        "%{prj.name}/spdlog/include/**.h",
        "%{prj.name}/flecs/distr/flecs.c",
        "%{prj.name}/flecs/distr/flecs.h",
    }
    
    includedirs
    {
        "%{IncludeDir.SPDLog}",
        "%{IncludeDir.Assimp}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.SDL}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.ECS}",
        "%{IncludeDir.dxc}",
        "%{IncludeDir.mono}",
        "%{IncludeDir.flecs}",
        "%{IncludeDir.rapidjson}",
    }
    
    libdirs
    {
        "%{prj.name}/assimp/lib",
        "%{prj.name}/SDL/lib",
        "%{prj.name}/dxc/lib",
        "%{prj.name}/mono/lib",
    }

    links
    {
        "SDL2", "SDL2main",
        "opengl32.lib",
        "dxcompiler.lib",
        "assimp-vc142-mt.lib",
        "d3d12", "dxgi", "d3dcompiler",
        "mono-2.0-sgen.lib",
    }

    filter { "files:**.cpp" }
        flags { "NoPCH" }

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

project "WaldemEngine"
    location "WaldemEngine"    
    kind "ConsoleApp"
    targetname "WaldemGame"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"
    dependson { "Vendor", "ScriptEngine" }

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    pchheader "wdpch.h"
    pchsource "WaldemEngine/src/wdpch.cpp"
    
    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "WD_EDITOR",
        'CONTENT_PATH=L"' .. contentPath .. '"'
    }

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.glsl",
        "%{prj.name}/src/**.hlsl",
        "Vendor/flecs/distr/flecs.c",
        "Vendor/flecs/distr/flecs.h",
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
        "%{IncludeDir.ECS}",
        "%{IncludeDir.dxc}",
        "%{IncludeDir.mono}",
        "%{IncludeDir.flecs}",
        "%{IncludeDir.rapidjson}",
    }

    links
    {
        "Vendor"
    }

    filter { "files:**flecs.**" }
        flags { "NoPCH" }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "WD_PLATFORM_WINDOWS",
            "WD_BUILD_DLL",
            "SDL_MAIN_HANDLED"
        }
        
        postbuildcommands
        {
            --Remove old shaders and content
            "if exist %{wks.location}bin\\Debug\\WaldemEngine\\Shaders (rmdir /s /q %{wks.location}bin\\Debug\\WaldemEngine\\Shaders)",
            "if exist %{wks.location}bin\\Debug\\WaldemEngine\\Content (rmdir /s /q %{wks.location}bin\\Debug\\WaldemEngine\\Content)",
            "if exist %{wks.location}bin\\Debug\\WaldemEngine\\mono (rmdir /s /q %{wks.location}bin\\Debug\\WaldemEngine\\mono)",
            "echo Copying files...",
            --Copy shaders
            'xcopy /E /I /Y "%{wks.location}%{prj.name}\\src\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',
            'xcopy /E /I /Y "%{wks.location}WaldemEngine\\src\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',
            --Copy dlls
            'xcopy /Y "%{wks.location}Vendor\\SDL\\lib\\SDL2.dll" "%{cfg.targetdir}\\"',
            'xcopy /Y "%{wks.location}Vendor\\mono\\bin\\mono-2.0-sgen.dll" "%{cfg.targetdir}\\"',
            'xcopy /E /I /Y "%{wks.location}Vendor\\mono\\lib\\4.5" "%{cfg.targetdir}\\mono\\lib\\4.5"', --Copy .NET assemblies
            'xcopy /Y "%{wks.location}Vendor\\dxc\\bin\\x64\\dxcompiler.dll" "%{cfg.targetdir}\\"',
            'xcopy /Y "%{wks.location}Vendor\\dxc\\bin\\x64\\dxil.dll" "%{cfg.targetdir}\\"',
            'xcopy /Y "%{wks.location}Vendor\\assimp\\lib\\assimp-vc142-mt.dll" "%{cfg.targetdir}\\"',
            --Copy content
            'xcopy /E /I /Y "%{wks.location}Content\\*" "%{cfg.targetdir}\\Content\\"'
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
        
project "ScriptEngine"
    location "ScriptEngine"
    kind "SharedLib" -- Could also be "ConsoleApp" or "WindowedApp"
    language "C#"

    targetdir ("bin/" .. outputdir .. "/WaldemEngine")

    files
    {
        "%{prj.name}/**.cs"
    }