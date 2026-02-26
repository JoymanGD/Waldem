#pragma once

#include "imgui_internal.h"

namespace ImGui
{
    inline bool SelectableInput(std::string& str_id, std::string& str, bool selected, ImGuiSelectableFlags flags, bool forceEdit = false)
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
        bool temp_input_start = forceEdit || (ret ? IsMouseDoubleClicked(0) : false);

        // Programmatic activation (e.g. F2 rename): request keyboard nav activation
        // in input-preferred mode so TempInputText can initialize correctly.
        if (forceEdit && !temp_input_is_active)
        {
            g.NavActivateId = id;
            g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
            temp_input_start = true;
        }

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

    inline void AssetInputSlot(Waldem::Path& inPathToAsset, const char* assetType, std::function<void()> onDropCallback = nullptr, const char* label = nullptr)
    {
        if(label)
        {
            PushID(label);
    
            Text("%s", label);
            SameLine();
        }
    
        Button(inPathToAsset.empty() ? "<None>" : inPathToAsset.string().c_str(), ImVec2(200, 0));
    
        // Accept drag and drop payload
        if (BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = AcceptDragDropPayload(assetType))
            {
                const char* pathToAsset = static_cast<const char*>(payload->Data);
                if (pathToAsset)
                {
                    inPathToAsset = Waldem::Path(pathToAsset);

                    if (onDropCallback)
                    {
                        onDropCallback();
                    }
                }
            }
            EndDragDropTarget();
        }
    
        // Optional: Right-click to clear
        if (BeginPopupContextItem())
        {
            if (MenuItem("Clear"))
                inPathToAsset.clear();
            EndPopup();
        }
    
        if(label)
        {
            PopID();
        }
    }
}
