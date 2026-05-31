project "imgui"
    location "."
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("../../Build/%{cfg.buildcfg}/%{prj.name}")
    objdir ("../../Intermediate/%{cfg.buildcfg}/%{prj.name}")

    files
    {
        "imgui.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
        "imgui_demo.cpp",
        "imgui_stdlib.cpp",
        "imgui_stdlib.h",
        "backends/imgui_impl_dx12.cpp",
        "backends/imgui_impl_sdl2.cpp",
        "imgui.h",
        "imgui_internal.h",
        "imconfig.h",
        "backends/imgui_impl_dx12.h",
        "backends/imgui_impl_sdl2.h",
    }

    includedirs
    {
        ".",
        "backends",
        "../SDL/include"
    }

    libdirs
    {
        "../SDL/lib"
    }

    links
    {
        "SDL2",
        "d3d12",
        "dxgi"
    }

    defines
    {
        "IMGUI_API=__declspec(dllexport)"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter {}
