workspace "Waldem"
    architecture "x64"
    startproject "WaldemEditor"

    configurations { "Debug", "Release", "Dist" }

    filter "system:windows"
        defines { "WIN32_LEAN_AND_MEAN", "_WINSOCKAPI_" }

outputdir = "%{cfg.buildcfg}"
local rootDir = os.getcwd()
local contentPath = path.getabsolute(path.join(rootDir, "Content"))

-----------------------------------
-- Include paths
-----------------------------------
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
IncludeDir["flecs"] = "Vendor/flecs"
IncludeDir["rapidjson"] = "Vendor/rapidjson/include"

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
        -- Remove old stuff
        "if exist %{cfg.targetdir}\\Shaders (rmdir /s /q %{cfg.targetdir}\\Shaders)",
        "if exist %{cfg.targetdir}\\Content (rmdir /s /q %{cfg.targetdir}\\Content)",
        "if exist %{cfg.targetdir}\\mono (rmdir /s /q %{cfg.targetdir}\\mono)",
        "echo Copying files...",

        -- Copy shaders
        'xcopy /E /I /Y "%{wks.location}WaldemEngine\\src\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',
        'xcopy /E /I /Y "%{wks.location}' .. prjName .. '\\src\\Shaders\\*.hlsl" "%{cfg.targetdir}\\Shaders\\"',

        -- Copy content
        'xcopy /E /I /Y "%{wks.location}Content\\*" "%{cfg.targetdir}\\Content\\"',
        'xcopy /E /I /Y "%{wks.location}' .. prjName .. '\\Content\\*" "%{cfg.targetdir}\\Content\\"',

        -- Copy DLLs
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
-- Common Helpers
-----------------------------------
function SetupCommonProject()
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    includedirs
    {
        "WaldemEngine/src",
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
        "%{IncludeDir.rapidjson}"
    }

    libdirs
    {
        "Vendor/assimp/lib",
        "Vendor/SDL/lib",
        "Vendor/dxc/lib",
        "Vendor/mono/lib"
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
        "mono-2.0-sgen.lib"
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

    filter "configurations:Dist"
        defines { "WD_DIST" }
        runtime "Release"
        optimize "on"

    filter {}
end

-----------------------------------
-- WaldemEngine (main DLL)
-----------------------------------
project "WaldemEngine"
    location "WaldemEngine"
    kind "SharedLib"

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")
    
    SetupCommonProject()

    pchheader "wdpch.h"
    pchsource "WaldemEngine/src/wdpch.cpp"

    defines
    {
        "WD_BUILD_DLL",
        "WD_DYNAMIC_LINK",
        "_CRT_SECURE_NO_WARNINGS",
        "IMGUI_API=__declspec(dllexport)"
    }

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.glsl",
        "%{prj.name}/src/**.hlsl",
        "Vendor/flecs/flecs.c",
        "Vendor/flecs/flecs.h",
        "Vendor/imgui/**.h",
        "Vendor/imgui/**.cpp",
        "Vendor/imgui/backends/**.h",
        "Vendor/imgui/backends/**.cpp",
        "Vendor/ImGuizmo/**.h",
        "Vendor/ImGuizmo/**.cpp"
    }

    filter "files:Vendor/**.cpp"
        flags { "NoPCH" }

    filter "files:Vendor/flecs/flecs.c"
        compileas "C"
        flags { "NoPCH" }

    filter "system:windows"
        defines { "SDL_MAIN_HANDLED" }

-----------------------------------
-- WaldemEditor (EXE)
-----------------------------------
project "WaldemEditor"
    location "WaldemEditor"
    kind "ConsoleApp"

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

    SetupCommonProject()

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    removefiles
    {
        "Vendor/imgui/misc/fonts/binary_to_compressed_c.cpp"
    }
    
    defines
    {
        "WD_DYNAMIC_LINK",
        "IMGUI_API=__declspec(dllimport)"
    }

    links { "WaldemEngine" }
    SetupPostBuild("WaldemEditor")

-----------------------------------
-- Sandbox (EXE)
-----------------------------------
project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"

    targetdir ("Build/" .. outputdir .. "/%{prj.name}")
    objdir ("Intermediate/" .. outputdir .. "/%{prj.name}")

    SetupCommonProject()

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
    }

    removefiles
    {
        "Vendor/imgui/misc/fonts/binary_to_compressed_c.cpp"
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
    location "ScriptEngine"
    kind "SharedLib"
    language "C#"

    targetdir ("Build/" .. outputdir .. "/WaldemEditor")

    files { "%{prj.name}/**.cs" }
