#include "wdpch.h"
#include "UIStyles.h"

#include "imgui.h"

namespace Waldem
{
    void UIStyles::ApplyDefault()
    {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        // Base colors for a pleasant and modern dark theme with dark accents
        colors[ImGuiCol_Text]                   = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.28f, 0.29f, 0.30f, 0.60f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.32f, 0.34f, 0.36f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.44f, 0.54f, 0.64f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.24f, 0.25f, 0.26f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.24f, 0.34f, 0.44f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.46f, 0.56f, 0.66f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        // --- Style adjustments for proper padding/spacing ---
        style->WindowPadding     = ImVec2(8.00f, 8.00f);
        style->FramePadding      = ImVec2(6.00f, 4.00f);
        style->CellPadding       = ImVec2(6.00f, 4.00f);
        style->ItemSpacing       = ImVec2(8.00f, 6.00f);
        style->ItemInnerSpacing  = ImVec2(6.00f, 4.00f);
        style->TouchExtraPadding = ImVec2(0.00f, 0.00f);

        style->WindowMenuButtonPosition = ImGuiDir_None;
        style->IndentSpacing            = 20.0f;
        style->ScrollbarSize            = 14.0f;
        style->GrabMinSize              = 12.0f;

        style->WindowBorderSize = 1.0f;
        style->ChildBorderSize  = 1.0f;
        style->PopupBorderSize  = 1.0f;
        style->FrameBorderSize  = 1.0f;
        style->TabBorderSize    = 1.0f;

        style->WindowRounding   = 6.0f;
        style->ChildRounding    = 4.0f;
        style->FrameRounding    = 3.0f;
        style->PopupRounding    = 4.0f;
        style->ScrollbarRounding= 9.0f;
        style->GrabRounding     = 3.0f;
        style->TabRounding      = 4.0f;
        style->LogSliderDeadzone= 4.0f;
    }

    void UIStyles::Apply(uint id)
    {
    }
}
