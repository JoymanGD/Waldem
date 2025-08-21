#pragma once

#include "Waldem/ECS/Components/Selected.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/Editor/AssetReference/AudioClipReference.h"
#include "Waldem/Extensions/ImGUIExtension.h"

namespace Waldem
{
    class WALDEM_API EntityDetailsWidgetContainer : public IWidget
    {
    private:
        ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    	CContentManager* ContentManager; //TODO: remove this dependency
        
    public:
        EntityDetailsWidgetContainer() {}

        WString GetName() override { return "Details"; }

    	void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
		{
			ContentManager = contentManager;
		}

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
                        	auto comp = ECS::RegisteredComponents[i].value;

                        	if(comp.has<EditorComponent>())
                        	{
                        		continue;
                        	}
                        	
                            if (strstr(compName, searchBuffer) != nullptr) // Filter by search
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

				if(component == ECS::World.id<SceneEntity>())
				{
					return;
				}

                void* ptr = entity.get_mut(id);

                flecs::entity comp = id.entity();

                auto componentName = comp.name().c_str();
                
                if (ImGui::BeginChild(componentName, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border, ImGuiWindowFlags_None))
                {
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
                    ImGui::TextUnformatted(componentName);
					ImGui::Separator();

					const EcsTypeSerializer* ser = ecs_get(ECS::World, comp, EcsTypeSerializer);
					auto v_ops = &ser->ops;
					ecs_meta_type_op_t* ops = ecs_vec_first_t(v_ops, ecs_meta_type_op_t);
					int op_count = ecs_vec_count(v_ops);

					ImGui::BeginTable("##PropertiesTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp);
					ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 120.0f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                	bool structDrawing = false;

					for (int i = 0; i < op_count; i++)
					{
						ecs_meta_type_op_t* op = &ops[i];

						if (op->name && strncmp(op->name, "___", 3) == 0)
						{
							continue;
						}

						switch (op->kind)
						{
							case EcsOpPush:
							{
								if (op->name)
								{
									ImGui::TableNextRow();
									ImGui::TableSetColumnIndex(0);
									ImGui::TextUnformatted(op->name);
									
									if (op->op_count - 2 == 3 &&
										ops[i + 1].kind == EcsOpF32 &&
										ops[i + 2].kind == EcsOpF32 &&
										ops[i + 3].kind == EcsOpF32)
									{
										ImGui::TableSetColumnIndex(1);
										structDrawing = true;
										WString name = WString("##") + op->name + WString::FromInt(op->member_index);
										ImGui::SetNextItemWidth(200);
										if(ImGui::DragFloat3(name, (float*)((uint8*)ptr + op->offset), 0.1f))
										{
											entity.modified(id);
										}
									}
								}
								break;
							}
							case EcsOpPop:
							{
								structDrawing = false;
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
								ImGui::TableNextRow();
								ImGui::TableSetColumnIndex(0);
								ImGui::Text("%s", op->name);

								// Pointer to the actual enum field in the component struct
								auto fieldPtr = reinterpret_cast<int*>((uint8_t*)ptr + op->offset);
								int currentValue = *fieldPtr;

								std::vector<const char*> names;
								flecs::entity enumType(ECS::World, op->type);
								if (enumType.has<EcsEnum>())
								{
									auto ecsEnum = enumType.get<EcsEnum>();
									auto constants = ecsEnum->constants;
									for (int j = 0; j < constants.bucket_count; ++j)
									{
										ecs_enum_constant_t* constant = (ecs_enum_constant_t*)ecs_map_get_deref_(&constants, j);
										names.push_back(constant->name);
									}
								}

								if (names.empty())
								{
									ImGui::TableSetColumnIndex(1);
									ImGui::Text("<no enum values>");
									break;
								}

								// Render combo
								ImGui::TableSetColumnIndex(1);
								ImGui::SetNextItemWidth(200.f);
								if (ImGui::Combo(("##" + std::string(op->name)).c_str(), &currentValue, names.data(), (int)names.size()))
								{
									*fieldPtr = currentValue;
									entity.modified(id);
								}
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
								ImGui::TableNextRow();
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
									entity.modified(id);
								}
								ImGui::PopID();
								break;
							}
							case EcsOpChar: break;
							case EcsOpByte: break;
							case EcsOpU8: break;
							case EcsOpU16: break;
							case EcsOpU32:
							{
								if(!structDrawing)
								{
									ImGui::TableNextRow();
									auto valPtr = (uint8*)ptr + op->offset;
									ImGui::PushID(ImGui::GetID(valPtr));

									ImGui::TableSetColumnIndex(0);
									ImGui::Text("%s", op->name);

									ImGui::TableSetColumnIndex(1);
									ImGui::SetNextItemWidth(200.f);
									if (ImGui::DragScalarN("##", ImGuiDataType_U32, valPtr, op->count, 1, NULL, NULL, "%llu"))
									{
										entity.modified(id);
									}

									ImGui::PopID();
								}
								break;
							}
							case EcsOpU64:
							{
								if(!structDrawing)
								{
									ImGui::TableNextRow();
									auto valPtr = (uint8*)ptr + op->offset;
									ImGui::PushID(ImGui::GetID(valPtr));

									ImGui::TableSetColumnIndex(0);
									ImGui::Text("%s", op->name);

									ImGui::TableSetColumnIndex(1);
									ImGui::SetNextItemWidth(200.f);
									if (ImGui::DragScalarN("##", ImGuiDataType_U64, valPtr, op->count, 1, NULL, NULL, "%llu"))
									{
										entity.modified(id);
									}

									ImGui::PopID();
								}
								break;
							}
							case EcsOpI8: break;
							case EcsOpI16: break;
							case EcsOpI32:
							{
								if(!structDrawing)
								{
									ImGui::TableNextRow();
									auto valPtr = (uint8*)ptr + op->offset;
									ImGui::PushID(ImGui::GetID(valPtr));

									ImGui::TableSetColumnIndex(0);
									ImGui::Text("%s", op->name);

									ImGui::TableSetColumnIndex(1);
									ImGui::SetNextItemWidth(200.f);
									if (ImGui::DragScalarN("##", ImGuiDataType_S32, valPtr, op->count, 1, NULL, NULL, "%d"))
									{
										entity.modified(id);
									}

									ImGui::PopID();
								}
								break;
							}
							case EcsOpI64: break;
							case EcsOpF32:
							{
								if(!structDrawing)
								{
									ImGui::TableNextRow();
									ImGui::TableSetColumnIndex(0);
									ImGui::TextUnformatted(op->name);
									
									ImGui::TableSetColumnIndex(1);
									auto valPtr = (uint8*)ptr + op->offset;
									ImGui::PushID(ImGui::GetID((void*)valPtr));
									if(ImGui::DragFloat(("##" + std::string(op->name)).c_str(), (float*)valPtr, 0.1f))
									{
										entity.modified(id);
									}
									ImGui::PopID();
								}
								break;
							}
							case EcsOpF64:
							{
								if(!structDrawing)
								{
									ImGui::TableNextRow();
									auto valPtr = (uint8*)ptr + op->offset;
									ImGui::PushID(ImGui::GetID((void*)valPtr));
									
									ImGui::TableSetColumnIndex(0);
									ImGui::Text("%s", op->name);
								
									ImGui::TableSetColumnIndex(1);
									ImGui::SetNextItemWidth(200.f);
									if(ImGui::DragScalarN("##", ImGuiDataType_Float, (float*)valPtr, op->count, 0.1f, NULL, NULL, "%.3f"))
									{
										entity.modified(id);
									}
									// ImGui::InputFloat(op->name, (float*)valPtr, 0.0f, 0.0f, "%.3f");
									ImGui::PopID();
								}
								break;
							}
							case EcsOpUPtr:
							{
								break;
							}
							case EcsOpIPtr: break;
							case EcsOpEntity:
							{
								ImGui::TableNextRow();
								ImGui::Text("TODO: Entity %s", op->name);
								break;
							}
							case EcsOpId: break;
							case EcsOpString:
							{
								break;
							}
							case EcsOpOpaque:
							{
								if(op->type == ECS::World.id<AssetReference>())
								{
									ImGui::TableNextRow();
									ImGui::TableSetColumnIndex(0);
									ImGui::Text("%s", op->name);
									void* fieldPtr = (uint8*)ptr + op->offset;
									AssetReference* assetRef = (AssetReference*)fieldPtr;
									WString myTextureID;
									ImGui::TableSetColumnIndex(1);
									auto assetType = assetRef->GetType();
									WString assetTypeString = AssetTypeToString(assetRef->GetType());
									ImGui::AssetInputSlot(assetRef->Reference, assetTypeString.C_Str(), [this, entity, id, assetRef]
									{
										entity.modified(id);
									});

									//DEBUG, TODO: clean later
									if(assetType == AssetType::Audio)
									{
										auto audioClipRef = (AudioClipReference*)assetRef;
										
										if(ImGui::Button("Play"))
										{
											Audio::Play(audioClipRef->Clip);
										}

										ImGui::SameLine();

										if(ImGui::Button("Stop"))
										{
											Audio::Stop(audioClipRef->Clip);
										}
									}
								}
								break;
							}
							default:
							{
								break;
							}
						}
					}
					ImGui::EndTable();

					ImGui::PopStyleVar(2);
                    
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
