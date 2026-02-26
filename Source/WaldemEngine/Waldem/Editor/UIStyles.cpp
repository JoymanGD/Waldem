#include "wdpch.h"
#include "UIStyles.h"

#include "imgui.h"

namespace Waldem
{
    void UIStyles::ApplyDefault()
    {
        ImGuiStyle* style = &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        // Neutral graphite base + restrained cyan accent.
        colors[ImGuiCol_Text]                   = ImVec4(0.91f, 0.94f, 0.97f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.48f, 0.53f, 0.58f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.10f, 0.12f, 0.14f, 0.98f);
        colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.24f, 0.28f, 0.85f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.16f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.18f, 0.24f, 0.29f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.07f, 0.09f, 0.11f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.09f, 0.13f, 0.16f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.07f, 0.09f, 0.11f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.09f, 0.12f, 0.15f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.22f, 0.28f, 0.33f, 0.85f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.30f, 0.38f, 0.45f, 0.95f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.36f, 0.45f, 0.53f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.96f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.28f, 0.58f, 0.86f, 0.90f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.33f, 0.67f, 0.96f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.14f, 0.19f, 0.23f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.20f, 0.29f, 0.36f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.23f, 0.34f, 0.42f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.21f, 0.30f, 0.37f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.24f, 0.35f, 0.44f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.22f, 0.27f, 0.32f, 0.80f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.33f, 0.67f, 0.96f, 0.90f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.33f, 0.67f, 0.96f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.20f, 0.28f, 0.34f, 0.50f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.30f, 0.45f, 0.56f, 0.85f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.33f, 0.67f, 0.96f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.10f, 0.13f, 0.16f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.20f, 0.29f, 0.36f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.15f, 0.23f, 0.29f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.09f, 0.12f, 0.15f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.13f, 0.18f, 0.23f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.33f, 0.67f, 0.96f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.56f, 0.82f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.28f, 0.58f, 0.86f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.56f, 0.82f, 1.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.10f, 0.13f, 0.16f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.19f, 0.25f, 0.30f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.15f, 0.20f, 0.24f, 1.00f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.09f, 0.11f, 0.14f, 0.55f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.11f, 0.14f, 0.17f, 0.55f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.41f, 0.59f, 0.55f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.56f, 0.82f, 1.00f, 0.95f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.33f, 0.67f, 0.96f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.07f, 0.09f, 0.11f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.20f, 0.48f, 0.72f, 0.70f);

        style->WindowPadding     = ImVec2(10.00f, 8.00f);
        style->FramePadding      = ImVec2(8.00f, 5.00f);
        style->CellPadding       = ImVec2(8.00f, 5.00f);
        style->ItemSpacing       = ImVec2(10.00f, 7.00f);
        style->ItemInnerSpacing  = ImVec2(7.00f, 5.00f);
        style->TouchExtraPadding = ImVec2(0.00f, 0.00f);

        style->WindowMenuButtonPosition = ImGuiDir_None;
        style->IndentSpacing            = 18.0f;
        style->ScrollbarSize            = 12.0f;
        style->GrabMinSize              = 11.0f;
        style->WindowTitleAlign         = ImVec2(0.02f, 0.5f);
        style->ButtonTextAlign          = ImVec2(0.5f, 0.5f);
        style->SelectableTextAlign      = ImVec2(0.0f, 0.5f);
        style->TabCloseButtonMinWidthUnselected = -1.0f;
        style->TabCloseButtonMinWidthSelected = -1.0f;
        style->TabBarBorderSize         = 1.0f;
        style->TabBarOverlineSize       = 2.0f;

        style->WindowBorderSize = 1.0f;
        style->ChildBorderSize  = 1.0f;
        style->PopupBorderSize  = 1.0f;
        style->FrameBorderSize  = 0.0f;
        style->TabBorderSize    = 0.0f;

        style->WindowRounding   = 7.0f;
        style->ChildRounding    = 6.0f;
        style->FrameRounding    = 5.0f;
        style->PopupRounding    = 6.0f;
        style->ScrollbarRounding= 7.0f;
        style->GrabRounding     = 5.0f;
        style->TabRounding      = 5.0f;
        style->LogSliderDeadzone= 4.0f;
    }

    void UIStyles::Apply(uint id)
    {
    }
}
