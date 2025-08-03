#pragma once

#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/ECS.h"

namespace Waldem
{
    class WALDEM_API EntityDetailsWidgetContainer : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        
    public:
        EntityDetailsWidgetContainer() {}

        WString GetName() override { return "Details"; }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Details", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                auto selectedEntities = ECS::World.query<Selected>();

                bool multiSelection = selectedEntities.count() > 1;

                selectedEntities.each([&](flecs::entity entity, Selected selected)
                {
                    if(multiSelection)
                    {
                        //TODO: add multi-selection handling
                    }
                    else
                    {
                        DrawComponents(entity);
                    }

                    if (ImGui::Button("Add component"))
                    {
                        ImGui::OpenPopup("AddComponentPopup");
                    }

                    if (ImGui::BeginPopup("AddComponentPopup"))
                    {
	                    static char searchBuffer[128] = "";
						ImGui::InputText("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));

                        for (int i = 0; i < ECS::RegisteredComponents.Num(); i++)
                        {
                        	auto compName = ECS::RegisteredComponents[i].key;
                            if (strstr(compName, searchBuffer) != nullptr) // Filter by search
                            {
                                if (ImGui::Selectable(compName, false))
                                {
									entity.add(ECS::RegisteredComponents[i].value);
                                	
									ImGui::CloseCurrentPopup();
                                }
                            }
                        }

                        ImGui::EndPopup();
                    }
                });
            }
            ImGui::End();
        }

        void DrawComponents(flecs::entity& entity)
        {
            entity.each([&](flecs::id id)
            {
                if(!id.is_entity())
                {
                    return;
                }

                auto component = ECS::World.component(id.type_id());

                if(component.raw_id() == 0)
                {
                    return;
                }

                if(!component.has<flecs::Type>())
                {
                    return;
                }

                void* ptr = entity.get_mut(id);

                flecs::entity comp = id.entity();

                auto componentName = comp.name().c_str();
                
                if (ImGui::BeginChild(componentName, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY))
                {
                    ImGui::Text(componentName);

					const EcsTypeSerializer* ser = ecs_get(ECS::World, comp, EcsTypeSerializer);
					auto v_ops = &ser->ops;
					ecs_meta_type_op_t* ops = ecs_vec_first_t(v_ops, ecs_meta_type_op_t);
					int op_count = ecs_vec_count(v_ops);

					int in_array = 0;

					ImGui::BeginTable("##PropertiesTable", 2);
					ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

					for (int i = 0; i < op_count; i++)
					{
						ecs_meta_type_op_t* op = &ops[i];

						if (op->kind != EcsOpPop)
						{
							ImGui::TableNextRow();
						}

						switch (op->kind)
						{
							case EcsOpPush:
							{
								if (op->name)
								{
									ImGui::TableSetColumnIndex(0);
									ImGui::Text("%s", op->name);
								}
								in_array--;
								break;
							}
							case EcsOpPop:
							{
								in_array++;
								break;
							}
							case EcsOpArray:
							{
								break;
							}
							case EcsOpVector:
							{
								break;
							}
							case EcsOpEnum:
							{
								// auto valPtr = (uint8*)ptr + op->offset;
								// ImGui::PushID(ImGui::GetID((void*)valPtr));
								// ecs::entity enumEntity = ecs.lookup(op->name);
								// if (enumEntity.is_valid())
								// {
								// 	int32 value = *(int32*)valPtr;
								// 	if (ImGui::Combo(op->name, &value, enumEntity.enum_values().c_str()))
								// 	{
								// 		*(int32*)valPtr = value;
								// 	}
								// }
								// else
								// {
								// 	ImGui::Text("Invalid enum type: %s", op->type.name().c_str());
								// }
								// ImGui::PopID();
								break;
							}
							case EcsOpBitmask:
							{
								break;
							}
							case EcsOpScope:
							{
								break;
							}
							case EcsOpPrimitive:
							{
								break;
							}
							case EcsOpBool:
							{
								ImGui::TableSetColumnIndex(0);
								ImGui::Text("%s", op->name);
								auto valPtr = (uint8*)ptr + op->offset;
								ImGui::PushID(ImGui::GetID((void*)valPtr));
								bool value = *(bool*)valPtr;
								ImGui::TableSetColumnIndex(1);

								WString name = WString("##") + op->name;
								if (ImGui::Checkbox(name, &value))
								{
									*(bool*)valPtr = value;
								}
								ImGui::PopID();
								break;
							}
							case EcsOpChar: break;
							case EcsOpByte: break;
							case EcsOpU8: break;
							case EcsOpU16: break;
							case EcsOpU32: break;
							case EcsOpU64: break;
							case EcsOpI8: break;
							case EcsOpI16: break;
							case EcsOpI32: break;
							case EcsOpI64: break;
							case EcsOpF32:
							{
								auto valPtr = (uint8*)ptr + op->offset;
								ImGui::PushID(ImGui::GetID((void*)valPtr));
								ImGui::TableSetColumnIndex(0);
								ImGui::Text("%s", op->name);
								ImGui::TableSetColumnIndex(1);
								ImGui::DragScalarN("##", ImGuiDataType_Float, (float*)valPtr, op->count, 0.1f, NULL, NULL, "%.3f");
								// ImGui::InputFloat(op->name, (float*)valPtr, 0.0f, 0.0f, "%.3f");
								ImGui::PopID();
								break;
							}
							case EcsOpF64:
							{
								auto valPtr = (uint8*)ptr + op->offset;
								ImGui::PushID(ImGui::GetID((void*)valPtr));
								ImGui::DragScalarN(op->name, ImGuiDataType_Double, (double*)valPtr, op->count, 0.1, NULL, NULL, "%.3f");
								ImGui::PopID();
								break;
							}
							case EcsOpUPtr: break;
							case EcsOpIPtr: break;
							case EcsOpEntity:
							{
								ImGui::Text("TODO: Entity %s", op->name);
							}
							case EcsOpId: break;
							case EcsOpString:
							{
								break;
							}
							case EcsOpOpaque: break;
							default:
							{
								break;
							}
						}
					}
					ImGui::EndTable();
                    
                    // if(componentWidget->IsRemovable() || componentWidget->IsResettable())
                    // {
                    //     ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                    //     if (ImGui::Button("..."))
                    //     {
                    //         ImGui::OpenPopup("ComponentContextMenu");
                    //     }
                    //
                    //     if (ImGui::BeginPopupContextItem("ComponentContextMenu", ImGuiPopupFlags_MouseButtonLeft))
                    //     {
                    //         if(componentWidget->IsResettable())
                    //         {
                    //             if (ImGui::MenuItem("Reset"))
                    //             {
                    //                 componentWidget->ResetComponent(entity);
                    //             }
                    //         }
                    //
                    //         if(componentWidget->IsRemovable())
                    //         {
                    //             if (ImGui::MenuItem("Remove"))
                    //             {
                    //                 componentWidget->RemoveComponent(entity);
                    //             }
                    //         }
                    //
                    //         ImGui::EndPopup();
                    //     }
                    // }
                }
                ImGui::EndChild();
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            });
        }
    };
}
