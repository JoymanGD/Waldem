project "ImGuizmo"
    location "."
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("../../Build/%{cfg.buildcfg}/%{prj.name}")
    objdir ("../../Intermediate/%{cfg.buildcfg}/%{prj.name}")

    files
    {
        "ImGuizmo.cpp",
        "ImGuizmo.h",
        "ImCurveEdit.h", "ImCurveEdit.cpp",
        "ImGradient.h", "ImGradient.cpp",
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
        "IMGUIZMO_API=",
        "IMGUI_API=__declspec(dllimport)"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Release"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter {}
