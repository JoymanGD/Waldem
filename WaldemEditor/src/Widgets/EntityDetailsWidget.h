#pragma once

#include "Widget.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/Editor/AssetReference/AudioClipReference.h"
#include "Waldem/Extensions/ImGUIExtension.h"
#include "Waldem/Input/InputManager.h"

namespace Waldem
{
    class EntityDetailsWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    public:
        EntityDetailsWidget() {}

        void Initialize(InputManager* inputManager) override
        {
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Details", nullptr,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                auto selectedEntities = ECS::World.query<Selected>();
                bool multiSelection = selectedEntities.count() > 1;

                ECS::World.defer_begin();
                selectedEntities.each([&](ECS::Entity entity, Selected)
                {
                    if (!multiSelection)
                        DrawComponents(entity);

                    if (ImGui::Button("Add component"))
                        ImGui::OpenPopup("AddComponentPopup");

                    if (ImGui::BeginPopup("AddComponentPopup"))
                    {
                        static char searchBuffer[128] = "";
                        ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));

                        for (int i = 0; i < ECS::RegisteredComponents.Num(); i++)
                        {
                            auto compName = ECS::RegisteredComponents[i].key;
                            auto comp = ECS::RegisteredComponents[i].value;

                            if (comp.has<EditorComponent>())
                                continue;

                            if (strstr(compName, searchBuffer) != nullptr)
                            {
                                if (ImGui::Selectable(compName, false))
                                {
                                    entity.add(comp);
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        }
                        ImGui::EndPopup();
                    }
                });
                ECS::World.defer_end();
            }
            ImGui::End();
        }

        void DrawComponentFields(flecs::entity entity, flecs::id id, void* base, flecs::meta::op_t* ops, int32_t op_count, const std::string& prefix = "")
        {
            for (int i = 0; i < op_count; i++)
            {
                flecs::meta::op_t* op = &ops[i];
                void* ptr = ECS_OFFSET(base, op->offset);

                if (!op->name || strncmp(op->name, "___", 3) == 0)
                    continue;

                std::string fullName = prefix.empty() ? op->name : prefix + "." + op->name;
                std::string uniqueId = "##" + fullName + "_" + std::to_string(id.raw_id()) + "_" + std::to_string(i);

                if (op->kind == EcsOpPushStruct)
                {
                    bool isNestedStruct = !prefix.empty();

                    if (!isNestedStruct)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(op->name);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::BeginGroup();
                    }

                    flecs::meta::op_t* inner_ops = ops + 1;
                    int inner_count = op->op_count - 2;

                    int drawn = 0;
                    for (int j = 0; j < inner_count; j++)
                    {
                        auto* inner = &inner_ops[j];
                        if (!inner->name || strncmp(inner->name, "___", 3) == 0)
                            continue;

                        void* innerPtr = ECS_OFFSET(ptr, inner->offset);
                        std::string innerId = uniqueId + "_" + std::string(inner->name) + "_" + std::to_string(j);

                        if (inner->kind == EcsOpPushStruct)
                        {
                            DrawComponentFields(entity, id, ptr, inner_ops + j, inner->op_count, fullName);
                            j += inner->op_count - 1;
                            continue;
                        }

                        if (drawn++ > 0)
                            ImGui::SameLine();

                        ImGui::TextUnformatted(inner->name);
                        ImGui::SameLine();

                        switch (inner->kind)
                        {
                            case EcsOpF32:
                            {
                                float v = *(float*)innerPtr;
                                ImGui::SetNextItemWidth(60.f);
                                if (ImGui::DragFloat(innerId.c_str(), &v, 0.1f))
                                {
                                    *(float*)innerPtr = v;
                                    entity.modified(id);
                                }
                                break;
                            }
                            case EcsOpF64:
                            {
                                double v = *(double*)innerPtr;
                                ImGui::SetNextItemWidth(60.f);
                                if (ImGui::DragScalar(innerId.c_str(), ImGuiDataType_Double, &v, 0.1f))
                                {
                                    *(double*)innerPtr = v;
                                    entity.modified(id);
                                }
                                break;
                            }
                            case EcsOpI32:
                            {
                                int32_t v = *(int32_t*)innerPtr;
                                ImGui::SetNextItemWidth(60.f);
                                if (ImGui::DragInt(innerId.c_str(), &v))
                                {
                                    *(int32_t*)innerPtr = v;
                                    entity.modified(id);
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }

                    if (!isNestedStruct)
                    {
                        ImGui::EndGroup();
                    }

                    i += op->op_count - 1;
                    continue;
                }

                if (op->kind != EcsOpPop &&
                    op->kind != EcsOpPushArray &&
                    op->kind != EcsOpPushVector)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(op->name);
                    ImGui::TableSetColumnIndex(1);
                }

                switch (op->kind)
                {
                    case EcsOpEnum:
                    {
                        int currentValue = 0;
                        memcpy(&currentValue, ptr, sizeof(int));

                        std::vector<const char*> names;
                        if (op->is.constants)
                        {
                            for (int j = 0; j < op->is.constants->bucket_count; ++j)
                            {
                                ecs_enum_constant_t* c =
                                    (ecs_enum_constant_t*)ecs_map_get_deref_(op->is.constants, j);
                                if (c && c->name)
                                    names.push_back(c->name);
                            }
                        }

                        if (!names.empty())
                        {
                            ImGui::SetNextItemWidth(200.f);
                            if (ImGui::Combo(uniqueId.c_str(), &currentValue, names.data(), (int)names.size()))
                            {
                                memcpy(ptr, &currentValue, sizeof(int));
                                entity.modified(id);
                            }
                        }
                        else
                        {
                            ImGui::TextUnformatted("<no enum constants>");
                        }
                        break;
                    }

                    case EcsOpBool:
                    {
                        bool value = *(bool*)ptr;
                        if (ImGui::Checkbox(uniqueId.c_str(), &value))
                        {
                            *(bool*)ptr = value;
                            entity.modified(id);
                        }
                        break;
                    }

                    case EcsOpI32:
                    {
                        int32_t val = *(int32_t*)ptr;
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragInt(uniqueId.c_str(), &val))
                        {
                            *(int32_t*)ptr = val;
                            entity.modified(id);
                        }
                        break;
                    }

                    case EcsOpU32:
                    {
                        uint32_t val = *(uint32_t*)ptr;
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragScalar(uniqueId.c_str(), ImGuiDataType_U32, &val, 1.0f))
                        {
                            *(uint32_t*)ptr = val;
                            entity.modified(id);
                        }
                        break;
                    }

                    case EcsOpF32:
                    {
                        float val = *(float*)ptr;
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragFloat(uniqueId.c_str(), &val, 0.1f))
                        {
                            *(float*)ptr = val;
                            entity.modified(id);
                        }
                        break;
                    }

                    case EcsOpF64:
                    {
                        double val = *(double*)ptr;
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragScalar(uniqueId.c_str(), ImGuiDataType_Double, &val, 0.1f))
                        {
                            *(double*)ptr = val;
                            entity.modified(id);
                        }
                        break;
                    }

                    case EcsOpOpaqueStruct:
                    case EcsOpOpaqueValue:
                    {
                        if (op->type == ECS::World.id<AssetReference>())
                        {
                            AssetReference* assetRef = (AssetReference*)((uint8_t*)base + op->offset);
                            WString assetTypeString = AssetTypeToString(assetRef->GetType());
                            ImGui::AssetInputSlot(assetRef->Reference, assetTypeString.C_Str(), [entity, id]()
                            {
                                entity.modified(id);
                            });

                            if (assetRef->GetType() == AssetType::Audio)
                            {
                                auto audioClipRef = (AudioClipReference*)assetRef;

                                if (ImGui::Button(("Play" + uniqueId).c_str()))
                                    Audio::Play(audioClipRef->Clip);

                                ImGui::SameLine();

                                if (ImGui::Button(("Stop" + uniqueId).c_str()))
                                    Audio::Stop(audioClipRef->Clip);
                            }
                        }
                        break;
                    }

                    default:
                        break;
                }

                i += op->op_count - 1;
            }
        }

        void DrawComponents(flecs::entity& entity)
        {
            entity.each([&](flecs::id id)
            {
                if (!id.is_entity())
                    return;

                flecs::entity comp = id.entity();

                if (comp == ECS::World.id<SceneEntity>())
                    return;

                if (comp.has<HiddenComponent>())
                    return;

                void* ptr = entity.get_mut(id);
                if (!ptr)
                    return;

                const char* componentName = comp.name().c_str();

                if (ImGui::BeginChild(componentName, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
                    ImGui::TextUnformatted(componentName);

                    ImGui::SameLine(ImGui::GetWindowWidth() - 60);
                    if (ImGui::SmallButton("..."))
                        ImGui::OpenPopup((std::string(componentName) + "_options").c_str());

                    if (ImGui::BeginPopup((std::string(componentName) + "_options").c_str()))
                    {
                        if (ImGui::MenuItem("Remove"))
                            entity.remove(id);
                        ImGui::EndPopup();
                    }

                    ImGui::Separator();

                    // === New Flecs Reflection Handling ===
                    if (comp.has<flecs::TypeSerializer>())
                    {
                        const auto& ts = comp.get<flecs::TypeSerializer>();
                        auto ops = ecs_vec_first_t(&ts.ops, flecs::meta::op_t);
                        int op_count = ecs_vec_count(&ts.ops);

                        ImGui::BeginTable("##PropertiesTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp);
                        ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                        DrawComponentFields(entity, id, ptr, ops, op_count);

                        ImGui::EndTable();
                    }

                    ImGui::PopStyleVar(2);
                }

                ImGui::EndChild();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            });
        }
    };
}
