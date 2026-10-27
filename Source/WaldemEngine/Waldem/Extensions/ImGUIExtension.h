#pragma once

#include <ImCurveEdit.h>
#include <ImGradient.h>

#include "imgui_internal.h"
#include "Waldem/ProjectManagement/ProjectBuilder.h"
#include "Waldem/Types/WCurve.h"
#include "Waldem/Types/WGradient.h"
#include "Waldem/ECS/ECSTypes.h"

namespace Waldem
{
    struct WGradient;
}

namespace ImGui
{
    struct ImGuiCurve : ImCurveEdit::Delegate
    {
        Waldem::WCurve& Curve;
        std::vector<ImVec2> Points;

        ImGuiCurve(Waldem::WCurve& curve) : Curve(curve)
        {
            Points.reserve(Curve.Count);

            for (Waldem::uint i = 0; i < Curve.Count; ++i)
            {
                const Waldem::Vector2& point = Curve.Points[i];
                Points.emplace_back(point.x, point.y);
            }
        }

        size_t GetCurveCount() override
        {
            return 1;
        }

        bool IsVisible(size_t) override
        {
            return true;
        }

        size_t GetPointCount(size_t) override
        {
            return Points.size();
        }

        uint32_t GetCurveColor(size_t) override
        {
            return IM_COL32(140, 220, 140, 255);
        }

        ImVec2* GetPoints(size_t) override
        {
            return Points.data();
        }

        ImCurveEdit::CurveType GetCurveType(size_t) const override
        {
            return ImCurveEdit::CurveSmooth;
        }

        int EditPoint(size_t, int index, ImVec2 value) override
        {
            Points[index] = ImVec2{value.x, value.y};

            std::sort(Points.begin(), Points.end(), [](const ImVec2& a, const ImVec2& b)
            {
                return a.x < b.x;
            });

            for (int i = 0; i < static_cast<int>(Points.size()); ++i)
            {
                if (Points[i].x == value.x && Points[i].y == value.y)
                    return i;
            }

            return index;
        }

        void AddPoint(size_t, ImVec2 value) override
        {
            Points.push_back({value.x, value.y});

            std::sort(Points.begin(), Points.end(), [](const ImVec2& a, const ImVec2& b)
            {
                return a.x < b.x;
            });
        }

        ImVec2& GetMin() override
        {
            return *reinterpret_cast<ImVec2*>(&Curve.Min);
        }

        ImVec2& GetMax() override
        {
            return *reinterpret_cast<ImVec2*>(&Curve.Max);
        }

        unsigned int GetBackgroundColor() override
        {
            return IM_COL32(25, 29, 34, 255);
        }

        void Apply()
        {
            Curve.Count = static_cast<Waldem::uint>(std::min(Points.size(), static_cast<size_t>(Waldem::WCurve::MaxPoints)));

            for (Waldem::uint i = 0; i < Curve.Count; ++i)
            {
                Curve.Points[i] = Waldem::Vector2(Points[i].x, Points[i].y);
            }

            for (Waldem::uint i = Curve.Count; i < Waldem::WCurve::MaxPoints; ++i)
            {
                Curve.Points[i] = Waldem::Vector2(0.0f);
            }
        }
    };
    
    struct ImGuiGradient : ImGradient::Delegate
    {
        Waldem::WGradient& Gradient;
        std::vector<ImVec4> Points;

        ImGuiGradient(Waldem::WGradient& gradient) : Gradient(gradient)
        {
            Points.reserve(Gradient.Count);

            for (Waldem::uint i = 0; i < Gradient.Count; ++i)
            {
                const Waldem::Vector4& point = Gradient.Points[i];
                Points.emplace_back(point.x, point.y, point.z, point.w);
            }
        }

        size_t GetPointCount() override
        {
            return Points.size();
        }

        ImVec4* GetPoints() override
        {
            return Points.data();
        }

        int EditPoint(int index, ImVec4 value) override
        {
            Points[index] = value;

            std::sort(Points.begin(), Points.end(), [](const ImVec4& a, const ImVec4& b)
            {
                return a.w < b.w;
            });

            for (int i = 0; i < static_cast<int>(Points.size()); ++i)
            {
                if (Points[i].x == value.x && Points[i].y == value.y && Points[i].z == value.z && Points[i].w == value.w)
                    return i;
            }

            return index;
        }

        ImVec4 GetPoint(float t) override
        {
            if (Points.empty())
                return ImVec4(1.0f, 1.0f, 1.0f, t);

            if (t <= Points.front().w)
            {
                ImVec4 result = Points.front();
                result.w = t;
                return result;
            }

            for (size_t i = 1; i < Points.size(); ++i)
            {
                const ImVec4& a = Points[i - 1];
                const ImVec4& b = Points[i];

                if (t <= b.w)
                {
                    float factor = b.w > a.w ? (t - a.w) / (b.w - a.w) : 0.0f;

                    return ImVec4(
                        a.x + (b.x - a.x) * factor,
                        a.y + (b.y - a.y) * factor,
                        a.z + (b.z - a.z) * factor,
                        t
                    );
                }
            }

            ImVec4 result = Points.back();
            result.w = t;
            return result;
        }

        void AddPoint(ImVec4 value) override
        {
            if (Points.size() >= Waldem::WGradient::MaxPoints)
                return;

            Points.push_back(value);

            std::sort(Points.begin(), Points.end(), [](const ImVec4& a, const ImVec4& b)
            {
                return a.w < b.w;
            });
        }

        void Apply()
        {
            Gradient.Count = static_cast<Waldem::uint>(std::min(Points.size(), static_cast<size_t>(Waldem::WGradient::MaxPoints)));

            for (Waldem::uint i = 0; i < Gradient.Count; ++i)
            {
                Gradient.Points[i] = Waldem::Vector4(Points[i].x, Points[i].y, Points[i].z, Points[i].w);
            }

            for (Waldem::uint i = Gradient.Count; i < Waldem::WGradient::MaxPoints; ++i)
            {
                Gradient.Points[i] = Waldem::Vector4(0.0f);
            }
        }
    };
    
    inline bool CurveField(const char* label, Waldem::WCurve& wCurve)
    {
        ImGuiCurve curve(wCurve);

        bool changed = false;
        
        PushID(label);

        ImVec2 size(180, 28);
        ImVec2 pos = GetCursorScreenPos();

        if (InvisibleButton("##CurvePreview", size))
            OpenPopup("##CurvePopup");

        ImDrawList* draw = GetWindowDrawList();

        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(30, 35, 40, 255), 4.0f);

        if (curve.GetCurveCount() > 0)
        {
            const size_t count = curve.GetPointCount(0);
            ImVec2* points = curve.GetPoints(0);

            ImVec2 min = curve.GetMin();
            ImVec2 max = curve.GetMax();

            float dx = max.x - min.x;
            float dy = max.y - min.y;

            if (count > 1 && dx > 0 && dy > 0)
            {
                draw->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);

                ImVec2 prev;

                for (size_t i = 0; i < count; ++i)
                {
                    float x = (points[i].x - min.x) / dx;
                    float y = (points[i].y - min.y) / dy;

                    ImVec2 current(pos.x + x * size.x, pos.y + (1.0f - y) * size.y);

                    if (i > 0)
                        draw->AddLine(prev, current, IM_COL32(130, 220, 130, 255), 2.0f);

                    prev = current;
                }

                draw->PopClipRect();
            }
        }

        if (BeginPopup("##CurvePopup"))
        {
            TextUnformatted("Curve Editor");
            Separator();

            if(Edit(curve, ImVec2(400, 240), GetID("##Editor")))
            {
                changed = true;
            }

            EndPopup();
        }

        if (changed)
        {
            curve.Apply();
        }

        PopID();

        return changed;
    }

    inline bool GradientField(const char* label, Waldem::WGradient& gradient)
    {
        PushID(label);

        bool changed = false;

        ImVec2 size(180, 32);
        ImVec2 pos = GetCursorScreenPos();

        if (InvisibleButton("##GradientPreview", size))
            OpenPopup("##GradientPopup");

        ImGuiGradient adapter(gradient);

        ImDrawList* draw = GetWindowDrawList();

        const int steps = 64;

        for (int i = 0; i < steps; ++i)
        {
            float t0 = static_cast<float>(i) / steps;
            float t1 = static_cast<float>(i + 1) / steps;

            ImVec4 color0 = adapter.GetPoint(t0);
            ImVec4 color1 = adapter.GetPoint(t1);

            draw->AddRectFilledMultiColor(
                ImVec2(pos.x + t0 * size.x, pos.y),
                ImVec2(pos.x + t1 * size.x, pos.y + size.y),
                ColorConvertFloat4ToU32(ImVec4(color0.x, color0.y, color0.z, 1.0f)),
                ColorConvertFloat4ToU32(ImVec4(color1.x, color1.y, color1.z, 1.0f)),
                ColorConvertFloat4ToU32(ImVec4(color1.x, color1.y, color1.z, 1.0f)),
                ColorConvertFloat4ToU32(ImVec4(color0.x, color0.y, color0.z, 1.0f))
            );
        }

        draw->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(100, 100, 100, 255));

        if (BeginPopup("##GradientPopup"))
        {
            TextUnformatted("Gradient Editor");
            Separator();

            static int selection = -1;

            if (ImGradient::Edit(adapter, ImVec2(400, 60), selection))
            {
                changed = true;
            }

            if (selection >= 0 && selection < static_cast<int>(adapter.Points.size()))
            {
                ImVec4& point = adapter.Points[selection];

                float color[3] = {point.x, point.y, point.z};

                if (ColorEdit3("Color", color))
                {
                    point.x = color[0];
                    point.y = color[1];
                    point.z = color[2];

                    changed = true;
                }
            }

            if (changed)
            {
                adapter.Apply();
            }

            EndPopup();
        }

        PopID();

        return changed;
    }
    
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

    inline void AssetInputSlot(Waldem::Path& inPathToAsset, const char* assetType, std::function<void()> onDropCallback = nullptr, const char* label = nullptr, std::function<void()> onClickCallback = nullptr)
    {
        PushID(&inPathToAsset);

        if(label)
        {
            PushID(label);
    
            Text("%s", label);
            SameLine();
        }

        // Keep label storage stable for the full call; avoid passing c_str() from a temporary path string.
        std::string buttonLabel = "<None>";
        if (!inPathToAsset.empty())
        {
            try
            {
                buttonLabel = inPathToAsset.generic_string();
            }
            catch (...)
            {
                buttonLabel = "<Invalid Path>";
            }
        }

        Button(buttonLabel.c_str(), ImVec2(200, 0));
        if (onClickCallback && IsItemClicked(ImGuiMouseButton_Left))
        {
            onClickCallback();
        }
    
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

        PopID();
    }

    inline void EntityInputSlot(Waldem::ECS::Entity& entity, std::function<void()> onDropCallback = nullptr, const char* label = nullptr, std::function<void()> onClickCallback = nullptr)
    {
        if(label)
        {
            PushID(label);
        } 

        std::string buttonLabel = "<None>";

        if(entity.is_alive())
        {
            buttonLabel = std::string(entity.name().c_str());
        }

        Button((buttonLabel + "###EntityInputSlot").c_str(), ImVec2(200, 0));
        
        if (onClickCallback && IsItemClicked(ImGuiMouseButton_Left))
        {
            onClickCallback();
        }
    
        // Accept drag and drop payload
        if (BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = AcceptDragDropPayload(Waldem::Payload::HierarchyDragPayloadType))
            {
                if(payload->IsDelivery() && payload->Data != nullptr && payload->DataSize == sizeof(Waldem::ECS::EntityT))
                {
                    const Waldem::ECS::EntityT entityId = *static_cast<const Waldem::ECS::EntityT*>(payload->Data);
                    Waldem::ECS::Entity droppedEntity = Waldem::ECS::World.entity(entityId);

                    if(droppedEntity.is_alive())
                    {
                        entity = droppedEntity;

                        if (onDropCallback)
                        {
                            onDropCallback();
                        }
                    }
                }
            }
            EndDragDropTarget();
        }
    
        // Optional: Right-click to clear
        if (BeginPopupContextItem())
        {
            if (MenuItem("Clear"))
            {
                entity = {};
            }
            EndPopup();
        }
    
        if(label)
        {
            PopID();
        }
    }
}
