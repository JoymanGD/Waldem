#pragma once

#include "Widget.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/Editor/AssetReference/AudioClipReference.h"
#include "Waldem/Editor/AssetReference/TextureReference.h"
#include "Waldem/Editor/AssetReference/MeshReference.h"
#include "Waldem/Editor/AssetReference/MaterialReference.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Extensions/ImGUIExtension.h"
#include "Waldem/Input/InputManager.h"
#include "Commands/EditorCommands.h"
#include "ContentBrowserWidget.h"

namespace Waldem
{
    class EntityDetailsWidget : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

        void PushComponentStateCommand(flecs::entity entity, flecs::id id, const ComponentValueBlob& before, const void* afterPtr, bool allowMerge = true)
        {
            ComponentValueBlob after(id.raw_id(), afterPtr);
            if(!before.IsValid() || !after.IsValid())
            {
                return;
            }

            if(before.Equals(after))
            {
                return;
            }

            EditorCommandHistory::Get().Execute(std::make_unique<SetComponentDataCommand>(entity.id(), id.raw_id(), before, after, allowMerge));
        }

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
                WArray<ECS::Entity> selected;
                selectedEntities.each([&](ECS::Entity entity, Selected)
                {
                    selected.Add(entity);
                });

                bool multiSelection = selected.Num() > 1;

                for (auto entity : selected)
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
                                    EditorCommandHistory::Get().Execute(std::make_unique<AddComponentCommand>(entity.id(), comp.id()));
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        }
                        ImGui::EndPopup();
                    }
                }
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

                    flecs::meta::op_t* inner_ops = op + 1;
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
                                ComponentValueBlob before(id.raw_id(), base);
                                ImGui::SetNextItemWidth(60.f);
                                if (ImGui::DragFloat(innerId.c_str(), &v, 0.1f))
                                {
                                    *(float*)innerPtr = v;
                                    entity.modified(id);
                                    PushComponentStateCommand(entity, id, before, base);
                                }
                                break;
                            }
                            case EcsOpF64:
                            {
                                double v = *(double*)innerPtr;
                                ComponentValueBlob before(id.raw_id(), base);
                                ImGui::SetNextItemWidth(60.f);
                                if (ImGui::DragScalar(innerId.c_str(), ImGuiDataType_Double, &v, 0.1f))
                                {
                                    *(double*)innerPtr = v;
                                    entity.modified(id);
                                    PushComponentStateCommand(entity, id, before, base);
                                }
                                break;
                            }
                            case EcsOpI32:
                            {
                                int32_t v = *(int32_t*)innerPtr;
                                ComponentValueBlob before(id.raw_id(), base);
                                ImGui::SetNextItemWidth(60.f);
                                if (ImGui::DragInt(innerId.c_str(), &v))
                                {
                                    *(int32_t*)innerPtr = v;
                                    entity.modified(id);
                                    PushComponentStateCommand(entity, id, before, base);
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
                        ComponentValueBlob before(id.raw_id(), base);

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
                                PushComponentStateCommand(entity, id, before, base);
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
                        ComponentValueBlob before(id.raw_id(), base);
                        if (ImGui::Checkbox(uniqueId.c_str(), &value))
                        {
                            *(bool*)ptr = value;
                            entity.modified(id);
                            PushComponentStateCommand(entity, id, before, base);
                        }
                        break;
                    }

                    case EcsOpI32:
                    {
                        int32_t val = *(int32_t*)ptr;
                        ComponentValueBlob before(id.raw_id(), base);
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragInt(uniqueId.c_str(), &val))
                        {
                            *(int32_t*)ptr = val;
                            entity.modified(id);
                            PushComponentStateCommand(entity, id, before, base);
                        }
                        break;
                    }

                    case EcsOpU32:
                    {
                        uint32_t val = *(uint32_t*)ptr;
                        ComponentValueBlob before(id.raw_id(), base);
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragScalar(uniqueId.c_str(), ImGuiDataType_U32, &val, 1.0f))
                        {
                            *(uint32_t*)ptr = val;
                            entity.modified(id);
                            PushComponentStateCommand(entity, id, before, base);
                        }
                        break;
                    }

                    case EcsOpF32:
                    {
                        float val = *(float*)ptr;
                        ComponentValueBlob before(id.raw_id(), base);
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragFloat(uniqueId.c_str(), &val, 0.1f))
                        {
                            *(float*)ptr = val;
                            entity.modified(id);
                            PushComponentStateCommand(entity, id, before, base);
                        }
                        break;
                    }

                    case EcsOpF64:
                    {
                        double val = *(double*)ptr;
                        ComponentValueBlob before(id.raw_id(), base);
                        ImGui::SetNextItemWidth(200.f);
                        if (ImGui::DragScalar(uniqueId.c_str(), ImGuiDataType_Double, &val, 0.1f))
                        {
                            *(double*)ptr = val;
                            entity.modified(id);
                            PushComponentStateCommand(entity, id, before, base);
                        }
                        break;
                    }

                    case EcsOpOpaqueStruct:
                    case EcsOpOpaqueValue:
                    {
                        const auto textureRefId = ECS::World.id<TextureReference>();
                        const auto meshRefId = ECS::World.id<MeshReference>();
                        const auto materialRefId = ECS::World.id<MaterialReference>();
                        const auto audioRefId = ECS::World.id<AudioClipReference>();
                        const auto baseAssetRefId = ECS::World.id<AssetReference>();

                        const bool isAssetReferenceType =
                            op->type == textureRefId ||
                            op->type == meshRefId ||
                            op->type == materialRefId ||
                            op->type == audioRefId ||
                            op->type == baseAssetRefId;

                        if (isAssetReferenceType)
                        {
                            AssetReference* assetRef = (AssetReference*)ptr;

                            AssetType assetType = AssetType::Unknown;
                            if (op->type == textureRefId) assetType = AssetType::Texture;
                            else if (op->type == meshRefId) assetType = AssetType::Mesh;
                            else if (op->type == materialRefId) assetType = AssetType::Material;
                            else if (op->type == audioRefId) assetType = AssetType::Audio;
                            else if (op->type == baseAssetRefId && op->name)
                            {
                                const std::string fieldName = op->name;
                                if (fieldName.find("Mesh") != std::string::npos) assetType = AssetType::Mesh;
                                else if (fieldName.find("Material") != std::string::npos) assetType = AssetType::Material;
                                else if (fieldName.find("Texture") != std::string::npos) assetType = AssetType::Texture;
                                else if (fieldName.find("Audio") != std::string::npos || fieldName.find("Clip") != std::string::npos) assetType = AssetType::Audio;
                            }

                            if (assetType == AssetType::Unknown)
                            {
                                ImGui::TextUnformatted("<Unsupported asset reference>");
                                break;
                            }

                            WString assetTypeString = AssetTypeToString(assetType);
                            ComponentValueBlob before(id.raw_id(), base);
                            const bool isMeshRefField =
                                (op->type == meshRefId) &&
                                op->name &&
                                (std::string(op->name) == "MeshRef");

                            ImGui::AssetInputSlot(assetRef->Reference, assetTypeString.C_Str(), [this, entity, id, base, before, isMeshRefField, op, materialRefId]()
                            {
                                if (isMeshRefField)
                                {
                                    MeshComponent* meshComponent = static_cast<MeshComponent*>(base);
                                    if (meshComponent)
                                    {
                                        const bool meshRefIsEmpty = meshComponent->MeshRef.Reference.empty() || meshComponent->MeshRef.Reference == "Empty";
                                        if (!meshRefIsEmpty)
                                        {
                                            if (!meshComponent->MeshRef.IsValid())
                                            {
                                                meshComponent->MeshRef.LoadAsset();
                                            }

                                            if (meshComponent->MeshRef.IsValid() && meshComponent->MeshRef.Mesh)
                                            {
                                                const MaterialReference& defaultMaterialRef = meshComponent->MeshRef.Mesh->MaterialRef;
                                                const bool materialRefIsEmpty = meshComponent->MaterialRef.Reference.empty() || meshComponent->MaterialRef.Reference == "Empty";
                                                const bool defaultMaterialIsValidPath = !defaultMaterialRef.Reference.empty() && defaultMaterialRef.Reference != "Empty";

                                                if (materialRefIsEmpty && defaultMaterialIsValidPath)
                                                {
                                                    meshComponent->MaterialRef.Reference = defaultMaterialRef.Reference;
                                                    meshComponent->MaterialRef.Mat = nullptr;
                                                }
                                            }
                                        }
                                    }
                                }
                                else if (op && op->type == materialRefId)
                                {
                                    MaterialReference* matRef = static_cast<MaterialReference*>(reinterpret_cast<void*>(ECS_OFFSET(base, op->offset)));
                                    if (matRef)
                                    {
                                        matRef->Mat = nullptr;
                                    }
                                }

                                entity.modified(id);
                                PushComponentStateCommand(entity, id, before, base, false);
                            }, nullptr, [assetRef, assetType]()
                            {
                                if (assetRef == nullptr)
                                {
                                    return;
                                }

                                Path focusPath = assetRef->Reference;
                                if (focusPath.empty() || focusPath == "Empty")
                                {
                                    return;
                                }

                                if (!focusPath.has_extension())
                                {
                                    focusPath.replace_extension(AssetTypeToExtension(assetType).ToString());
                                }

                                ContentBrowserWidget::FocusAssetPath(focusPath);
                            });

                            if (op->type == audioRefId)
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
            std::vector<flecs::id> componentIds;
            entity.each([&](flecs::id id)
            {
                if (!id.is_entity())
                    return;

                flecs::entity comp = id.entity();

                if (comp == ECS::World.id<SceneEntity>())
                    return;

                if (comp.has<HiddenComponent>())
                    return;

                componentIds.push_back(id);
            });

            for (auto id : componentIds)
            {
                if(!entity.is_alive() || !entity.has(id))
                {
                    continue;
                }

                flecs::entity comp = id.entity();

                void* ptr = entity.get_mut(id);
                if (!ptr)
                    continue;

                const char* componentName = comp.name().c_str();
                bool removeComponent = false;
                ImGui::PushID((int)id.raw_id());

                if (ImGui::BeginChild(componentName, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
                    ImGui::BeginTable("##ComponentHeader", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadInnerX);
                    ImGui::TableSetupColumn("##Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("##Action", ImGuiTableColumnFlags_WidthFixed, 24.0f);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(componentName);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::ArrowButton("##ComponentOptionsButton", ImGuiDir_Down);
                    ImGui::EndTable();

                    if (ImGui::BeginPopupContextItem("##ComponentOptionsPopup", ImGuiPopupFlags_MouseButtonLeft))
                    {
                        if (ImGui::MenuItem("Remove"))
                            removeComponent = true;
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

                if(removeComponent)
                {
                    EditorCommandHistory::Get().Execute(std::make_unique<RemoveComponentCommand>(entity, id.raw_id()));
                }

                ImGui::PopID();
            }
        }
    };
}
