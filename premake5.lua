workspace "Waldem"
    architecture "x64"
    startproject "WaldemEditor"

    configurations { "Debug", "Release" }

    filter "system:windows"
        defines { "WIN32_LEAN_AND_MEAN", "_WINSOCKAPI_" }

outputdir = "%{cfg.buildcfg}"
local rootDir = os.getcwd()
local contentPath = path.getabsolute(path.join(rootDir, "Content"))

-----------------------------------
-- Global Source Folder
-----------------------------------
SourceDir = "Source"

-----------------------------------
-- Include paths
-----------------------------------
IncludeDir = {}
IncludeDir["ImGui"]     = "Vendor/imgui"
IncludeDir["ImGuizmo"]  = "Vendor/ImGuizmo"
IncludeDir["glm"]       = "Vendor/glm"
IncludeDir["SDL"]       = "Vendor/SDL/include"
IncludeDir["SPDLog"]    = "Vendor/spdlog/include"
IncludeDir["Assimp"]    = "Vendor/assimp/include"
IncludeDir["stb"]       = "Vendor/stb/include"
IncludeDir["dxc"]       = "Vendor/dxc/inc"
IncludeDir["mono"]      = "Vendor/mono/include"
IncludeDir["flecs"]     = "Vendor/flecs"
IncludeDir["rapidjson"] = "Vendor/rapidjson/include"
IncludeDir["Generated"] = "Intermediate/Generated"

-----------------------------------
-- Shader filters
-----------------------------------
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

-----------------------------------
-- Shared Post-Build function
-----------------------------------
function SetupPostBuild(prjName)
    postbuildcommands {
        "if exist %{cfg.targetdir}\\Shaders (rmdir /s /q %{cfg.targetdir}\\Shaders)",
        "if exist %{cfg.targetdir}\\Content (rmdir /s /q %{cfg.targetdir}\\Content)",
        "if exist %{cfg.targetdir}\\mono (rmdir /s /q %{cfg.targetdir}\\mono)",

        "echo Copying files...",

        -- Shaders
        'xcopy /E /I /Y "%{wks.location}Source\\WaldemEngine\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',
        'xcopy /E /I /Y "%{wks.location}Source\\' .. prjName .. '\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',

        -- Content
        'xcopy /E /I /Y "%{wks.location}Content\\*" "%{cfg.targetdir}\\Content\\"',

        -- DLLs
        'xcopy /Y "%{wks.location}Build\\%{cfg.buildcfg}\\WaldemEngine\\WaldemEngine.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\SDL\\lib\\SDL2.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\mono\\bin\\mono-2.0-sgen.dll" "%{cfg.targetdir}\\"',
        'xcopy /E /I /Y "%{wks.location}Vendor\\mono\\lib\\4.5" "%{cfg.targetdir}\\mono\\lib\\4.5"',
        'xcopy /Y "%{wks.location}Vendor\\dxc\\bin\\x64\\dxcompiler.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\dxc\\bin\\x64\\dxil.dll" "%{cfg.targetdir}\\"',
        'xcopy /Y "%{wks.location}Vendor\\assimp\\lib\\assimp-vc142-mt.dll" "%{cfg.targetdir}\\"'
    }
end

-----------------------------------
-- Common C++ Settings
-----------------------------------
function SetupCommonProject()
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
        "%{IncludeDir.flecs}",
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
        "SDL2",
        "opengl32.lib",
        "dxcompiler.lib",
        "assimp-vc142-mt.lib",
        "d3d12",
        "dxgi",
        "d3dcompiler",
        "mono-2.0-sgen.lib",
    }

    defines { 'CONTENT_PATH=L"' .. contentPath .. '"' }

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

------------------------------------
-- WaldemHeaderTool
------------------------------------
project "WaldemHeaderTool"
    location (SourceDir .. "/WaldemHeaderTool")
    kind "ConsoleApp"
    cppdialect "C++20"

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

    files { SourceDir .. "/WaldemHeaderTool/**.cpp" }

-----------------------------------
-- WaldemEngine (DLL)
-----------------------------------
project "WaldemEngine"
    location (SourceDir .. "/WaldemEngine")
    kind "SharedLib"

    links { "WaldemHeaderTool" }

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

    SetupCommonProject()

    pchheader "wdpch.h"
    pchsource (SourceDir .. "/WaldemEngine/wdpch.cpp")

    defines
    {
        "WD_BUILD_DLL",
        "WD_DYNAMIC_LINK",
        "_CRT_SECURE_NO_WARNINGS",
        "IMGUI_API=__declspec(dllexport)"
    }

    files
    {
        SourceDir .. "/WaldemEngine/**.h",
        SourceDir .. "/WaldemEngine/**.cpp",
        "Intermediate/Generated/**.generated.h",
        SourceDir .. "/WaldemEngine/Shaders/**.hlsl",

        "Vendor/flecs/flecs.c",
        "Vendor/flecs/flecs.h",
        "Vendor/imgui/**.h",
        "Vendor/imgui/**.cpp",
        "Vendor/imgui/backends/**.h",
        "Vendor/imgui/backends/**.cpp",
        "Vendor/ImGuizmo/**.h",
        "Vendor/ImGuizmo/**.cpp"
    }

    prebuildcommands {
        "\"%{wks.location}Build\\%{cfg.buildcfg}\\WaldemHeaderTool\\WaldemHeaderTool.exe\" " ..
        "\"%{wks.location}Source\\WaldemEngine\" " ..
        "\"%{wks.location}Intermediate\\Generated\""
    }

    filter "files:Vendor/**.cpp"
        flags { "NoPCH" }

    filter "files:Vendor/flecs/flecs.c"
        compileas "C"
        flags { "NoPCH" }

-----------------------------------
-- WaldemEditor
-----------------------------------
project "WaldemEditor"
    location (SourceDir .. "/WaldemEditor")
    kind "ConsoleApp"

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")
    debugdir ("Build/" .. outputdir .. "/%{prj.name}")
    
    SetupCommonProject()

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

    links { "WaldemEngine" }
    SetupPostBuild("WaldemEditor")

-----------------------------------
-- Sandbox
-----------------------------------
project "Sandbox"
    location (SourceDir .. "/Sandbox")
    kind "ConsoleApp"

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")
    debugdir ("Build/" .. outputdir .. "/%{prj.name}")

    SetupCommonProject()

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

    links { "WaldemEngine" }
    SetupPostBuild("Sandbox")

-----------------------------------
-- ScriptEngine (C#)
-----------------------------------
project "ScriptEngine"
    location (SourceDir .. "/ScriptEngine")
    kind "SharedLib"
    language "C#"

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

    files { SourceDir .. "/ScriptEngine/**.cs" }