#pragma once

#include "imgui_internal.h"

namespace ImGui
{
    inline bool SelectableInput(std::string& str_id, std::string& str, bool selected, ImGuiSelectableFlags flags)
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImVec2 pos = window->DC.CursorPos;

        PushID(str_id.c_str());
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(g.Style.ItemSpacing.x, g.Style.FramePadding.y * 2.0f));
        bool ret = Selectable("##Selectable", selected, flags | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowOverlap);
        PopStyleVar();

        ImGuiID id = window->GetID("##Input");
        bool temp_input_is_active = TempInputIsActive(id);
        bool temp_input_start = ret ? IsMouseDoubleClicked(0) : false;

        if (temp_input_is_active || temp_input_start)
        {
            constexpr size_t maxLen = 256;
            char buf[maxLen];
            std::strncpy(buf, str.c_str(), maxLen);
            buf[maxLen - 1] = '\0'; // ensure null-terminated

            ImGuiInputTextFlags input_flags = flags | ImGuiInputTextFlags_EnterReturnsTrue;
            bool edited = TempInputText(g.LastItemData.Rect, id, "##Input", buf, static_cast<int>(maxLen), input_flags);
    
            if (edited)
            {
                str = std::string(buf);  // overwrite with trimmed content
                ret = true;
            }

            KeepAliveID(id);
        }
        else
        {
            window->DrawList->AddText(pos, GetColorU32(ImGuiCol_Text), str.c_str());
        }

        PopID();
        return ret;
    }

    inline bool SocketSlot(const char* id, bool filled = false, ImU32 color = IM_COL32(200, 200, 200, 255))
    {
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImVec2 slotCenter = ImVec2(cursorPos.x + 6, cursorPos.y + 6);
        float radius = 6.0f;

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Optional: hover highlight
        bool hovered = ImGui::IsItemHovered();

        drawList->AddCircleFilled(slotCenter, radius, IM_COL32(40, 40, 40, 255));
        if (filled) {
            drawList->AddCircleFilled(slotCenter, radius * 0.6f, color);
        } else {
            drawList->AddCircle(slotCenter, radius, color, 16, 2.0f);
        }

        // Create an invisible button to handle interaction
        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y));
        ImGui::InvisibleButton(id, ImVec2(radius * 2, radius * 2));

        // Drag-drop support
        bool changed = false;
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SOCKET_PAYLOAD")) {
                // process payload
                changed = true;
            }
            ImGui::EndDragDropTarget();
        }

        return changed;
    }
}