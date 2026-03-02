workspace "Waldem"
    architecture "x64"
    startproject "WaldemEditor"
    configurations { "Debug", "Release" }

    filter "system:windows"
        defines { "WIN32_LEAN_AND_MEAN", "_WINSOCKAPI_" }
    filter {}

local SourceDir = "Source"
local OutputDir = "%{cfg.buildcfg}"
local rootDir = os.getcwd()
local contentPath = path.getabsolute(path.join(rootDir, "Content"))

IncludeDir = {}
IncludeDir["ImGui"] = "Vendor/imgui"
IncludeDir["ImGuizmo"] = "Vendor/ImGuizmo"
IncludeDir["glm"] = "Vendor/glm"
IncludeDir["SDL"] = "Vendor/SDL/include"
IncludeDir["SPDLog"] = "Vendor/spdlog/include"
IncludeDir["Assimp"] = "Vendor/assimp/include"
IncludeDir["stb"] = "Vendor/stb/include"
IncludeDir["dxc"] = "Vendor/dxc/inc"
IncludeDir["mono"] = "Vendor/mono/include"
IncludeDir["flecs"] = "Vendor/flecs/include"
IncludeDir["rapidjson"] = "Vendor/rapidjson/include"
IncludeDir["Generated"] = "Intermediate/Generated"
IncludeDir["TinyCudaNN"] = "Vendor/tinycudann/include"
IncludeDir["TinyCudaNNDep"] = "Vendor/tinycudann/dependencies"

local function SetDefaultPaths()
    targetdir ("Build/" .. OutputDir .. "/%{prj.name}")
    objdir ("Intermediate/" .. OutputDir .. "/%{prj.name}")
end

local function SetupCommonCppProject()
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    includedirs
    {
        SourceDir .. "/WaldemEngine",
        "%{IncludeDir.SPDLog}",
        "%{IncludeDir.Assimp}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.SDL}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.dxc}",
        "%{IncludeDir.mono}",
        "%{IncludeDir.rapidjson}",
        "%{IncludeDir.Generated}",
    }

    libdirs
    {
        "Vendor/assimp/lib",
        "Vendor/SDL/lib",
        "Vendor/dxc/lib",
        "Vendor/mono/lib",
    }

    links
    {
        "imgui",
        "SDL2",
        "opengl32.lib",
        "dxcompiler.lib",
        "assimp-vc142-mt.lib",
        "d3d12",
        "dxgi",
        "d3dcompiler",
        "mono-2.0-sgen.lib",
    }

    defines
    {
        'CONTENT_PATH=L"' .. contentPath .. '"'
    }

    filter "system:windows"
        systemversion "latest"
        defines { "WD_PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        defines { "WD_DEBUG" }
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines { "WD_RELEASE" }
        runtime "Release"
        optimize "on"

    filter {}
end

local function SetupPostBuild(prjName)
    postbuildcommands
    {
        "if exist %{cfg.targetdir}\\Shaders (rmdir /s /q %{cfg.targetdir}\\Shaders)",
        "if exist %{cfg.targetdir}\\Content (rmdir /s /q %{cfg.targetdir}\\Content)",
        "if exist %{cfg.targetdir}\\mono (rmdir /s /q %{cfg.targetdir}\\mono)",

        "echo Copying files...",

        'xcopy /E /I /Y "%{wks.location}Source\\WaldemEngine\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',
        'xcopy /E /I /Y "%{wks.location}Source\\' .. prjName .. '\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',
        'xcopy /E /I /Y "%{wks.location}Content\\*" "%{cfg.targetdir}\\Content\\"',

        'xcopy /Y "%{wks.location}Build\\%{cfg.buildcfg}\\WaldemEngine\\WaldemEngine.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Build\\%{cfg.buildcfg}\\flecs\\flecs.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Build\\%{cfg.buildcfg}\\imgui\\imgui.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Build\\%{cfg.buildcfg}\\ImGuizmo\\ImGuizmo.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\SDL\\lib\\SDL2.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\mono\\bin\\mono-2.0-sgen.dll" "%{cfg.targetdir}\\"',
        'xcopy /E /I /Y "%{wks.location}Vendor\\mono\\lib\\4.5" "%{cfg.targetdir}\\mono\\lib\\4.5"',
        'xcopy /Y "%{wks.location}Vendor\\dxc\\bin\\x64\\dxcompiler.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\dxc\\bin\\x64\\dxil.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\assimp\\lib\\assimp-vc142-mt.dll" "%{cfg.targetdir}\\"'
    }
end

group "Vendor"
include "Vendor/flecs"
include "Vendor/imgui"
include "Vendor/ImGuizmo"
group ""

project "WaldemHeaderTool"
    location (SourceDir .. "/WaldemHeaderTool")
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    SetDefaultPaths()

    files
    {
        SourceDir .. "/WaldemHeaderTool/**.cpp"
    }

project "WaldemCoach"
    location (SourceDir .. "/WaldemCoach")
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    SetDefaultPaths()

    files
    {
        SourceDir .. "/WaldemCoach/**.h",
        SourceDir .. "/WaldemCoach/**.cpp"
    }

    includedirs
    {
        "$(CUDA_PATH)/include",
        "%{IncludeDir.TinyCudaNN}",
        "%{IncludeDir.TinyCudaNNDep}",
    }

    libdirs
    {
        "Vendor/tinycudann/build/Release",
        "$(CUDA_PATH)/lib/x64"
    }

    links
    {
        "tiny-cuda-nn",
        "cudart"
    }

project "WaldemEngine"
    location (SourceDir .. "/WaldemEngine")
    kind "SharedLib"
    SetDefaultPaths()
    SetupCommonCppProject()

    pchheader "wdpch.h"
    pchsource (SourceDir .. "/WaldemEngine/wdpch.cpp")

    dependson { "WaldemHeaderTool" }

    includedirs
    {
        "%{IncludeDir.flecs}"
    }

    files
    {
        SourceDir .. "/WaldemEngine/**.h",
        SourceDir .. "/WaldemEngine/**.cpp",
        SourceDir .. "/WaldemEngine/Shaders/**.hlsl"
    }

    links
    {
        "flecs"
    }

    defines
    {
        "WD_BUILD_DLL",
        "WD_DYNAMIC_LINK",
        "_CRT_SECURE_NO_WARNINGS",
        "IMGUI_API=__declspec(dllimport)"
    }

    prebuildcommands
    {
        "\"%{wks.location}Build\\%{cfg.buildcfg}\\WaldemHeaderTool\\WaldemHeaderTool.exe\" " ..
        "\"%{wks.location}Source\\WaldemEngine\" " ..
        "\"%{wks.location}Intermediate\\Generated\""
    }

    filter "files:**.hlsl"
        flags { "ExcludeFromBuild", "NoPCH" }
        shadermodel "6.6"

    filter "files:**.ps.hlsl"
        shadertype "Pixel"
        shaderentry "main"

    filter "files:**.vs.hlsl"
        shadertype "Vertex"
        shaderentry "main"

    filter "files:**.comp.hlsl"
        shadertype "Compute"
        shaderentry "main"

    filter {}

project "WaldemEditor"
    location (SourceDir .. "/WaldemEditor")
    kind "ConsoleApp"
    SetDefaultPaths()
    debugdir ("Build/" .. OutputDir .. "/%{prj.name}")
    SetupCommonCppProject()

    files
    {
        SourceDir .. "/WaldemEditor/**.h",
        SourceDir .. "/WaldemEditor/**.cpp",
    }

    defines
    {
        "WD_DYNAMIC_LINK",
        "IMGUI_API=__declspec(dllimport)"
    }

    links
    {
        "WaldemEngine",
        "flecs",
        "ImGuizmo"
    }

    SetupPostBuild("WaldemEditor")

project "Sandbox"
    location (SourceDir .. "/Sandbox")
    kind "ConsoleApp"
    SetDefaultPaths()
    debugdir ("Build/" .. OutputDir .. "/%{prj.name}")
    SetupCommonCppProject()

    files
    {
        SourceDir .. "/Sandbox/**.h",
        SourceDir .. "/Sandbox/**.cpp",
    }

    defines
    {
        "WD_DYNAMIC_LINK",
        "IMGUI_API=__declspec(dllimport)"
    }

    links
    {
        "WaldemEngine",
        "flecs"
    }

    SetupPostBuild("Sandbox")

project "ScriptEngine"
    location (SourceDir .. "/ScriptEngine")
    kind "SharedLib"
    language "C#"
    SetDefaultPaths()

    files
    {
        SourceDir .. "/ScriptEngine/**.cs"
    }
