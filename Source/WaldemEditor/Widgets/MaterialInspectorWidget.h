#pragma once

#include "Widget.h"
#include "ContentBrowserWidget.h"
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/Extensions/ImGUIExtension.h"
#include "Waldem/Renderer/Model/Material.h"
#include "Waldem/Utils/DataUtils.h"
#include <fstream>
#include <algorithm>

namespace Waldem
{
    class MaterialInspectorWidget : public IWidget
    {
    private:
        Path LoadedPath = "";
        Material* LoadedMaterial = nullptr;
        bool Dirty = false;

        void UnloadMaterial()
        {
            if (LoadedMaterial)
            {
                delete LoadedMaterial;
                LoadedMaterial = nullptr;
            }

            LoadedPath.clear();
            Dirty = false;
        }

        void LoadMaterial(const Path& materialPath)
        {
            UnloadMaterial();

            Path loadPath = materialPath;
            if (loadPath.is_relative())
            {
                std::error_code ec;
                Path rel = std::filesystem::relative(loadPath, CONTENT_PATH, ec);
                if (!ec && !rel.empty() && rel.string().find("..") != 0)
                {
                    loadPath = rel;
                }
            }

            LoadedMaterial = CContentManager::LoadAsset<Material>(loadPath);
            if (LoadedMaterial)
            {
                LoadedPath = materialPath;
            }
        }

        bool SaveLoadedMaterial()
        {
            if (!LoadedMaterial || LoadedPath.empty())
            {
                return false;
            }

            WDataBuffer outData;
            LoadedMaterial->Serialize(outData);

            uint64 hash = HashFromData(outData.GetData(), outData.GetSize());
            outData.Prepend(&hash, sizeof(uint64));

            std::ofstream outFile(LoadedPath.c_str(), std::ios::binary | std::ios::trunc);
            if (!outFile.is_open())
            {
                return false;
            }

            outFile.write((const char*)outData.GetData(), outData.GetSize());
            outFile.close();

            auto normalizeMaterialPath = [](Path path)
            {
                if (path.empty() || path == "Empty")
                {
                    return std::string();
                }

                path.replace_extension(".mat");
                if (path.is_relative())
                {
                    path = Path(CONTENT_PATH) / path;
                }

                std::error_code ec;
                Path canonical = std::filesystem::weakly_canonical(path, ec);
                if (!ec)
                {
                    path = canonical;
                }

                std::string s = path.generic_string();
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                return s;
            };

            const std::string targetMaterialPath = normalizeMaterialPath(LoadedPath);

            ECS::World.query<MeshComponent>().each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                if (!meshComponent.MeshRef.IsValid() || !meshComponent.MeshRef.Mesh)
                {
                    return;
                }

                const std::string meshMaterialPath = normalizeMaterialPath(meshComponent.MeshRef.Mesh->MaterialRef.Reference);
                const std::string componentMaterialPath = normalizeMaterialPath(meshComponent.MaterialRef.Reference);

                bool usesTargetMaterial = (meshMaterialPath == targetMaterialPath) || (componentMaterialPath == targetMaterialPath);
                if (!usesTargetMaterial)
                {
                    return;
                }

                // Force material reload from disk for runtime mesh instances.
                if (meshMaterialPath == targetMaterialPath)
                {
                    meshComponent.MeshRef.Mesh->MaterialRef.LoadAsset();
                }

                if (componentMaterialPath == targetMaterialPath)
                {
                    meshComponent.MaterialRef.LoadAsset();
                }

                entity.modified<MeshComponent>();
            });

            Dirty = false;
            return true;
        }

    public:
        MaterialInspectorWidget() = default;

        void Deinitialize() override
        {
            UnloadMaterial();
        }

        void OnDraw(float deltaTime) override
        {
            if (ImGui::Begin("Material Inspector", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
            {
                const auto& selectedPath = ContentBrowserWidget::GetSelectedAssetPath();
                bool selectedMaterial = selectedPath.has_value() && selectedPath->extension() == ".mat";

                if (selectedMaterial)
                {
                    if (!LoadedMaterial || LoadedPath != selectedPath.value())
                    {
                        LoadMaterial(selectedPath.value());
                    }
                }

                if (!LoadedMaterial)
                {
                    ImGui::TextUnformatted("Select a .mat asset in Content to inspect it.");
                }
                else
                {
                    ImGui::Text("Asset: %s", LoadedPath.filename().string().c_str());
                    ImGui::Separator();

                    ImGui::TextUnformatted("Textures");
                    Path& diffuseRef = LoadedMaterial->GetDiffuseReference();
                    Path& normalRef = LoadedMaterial->GetNormalReference();
                    Path& ormRef = LoadedMaterial->GetORMReference();

                    auto focusTextureAsset = [](const Path& textureRef)
                    {
                        if (textureRef.empty() || textureRef == "Empty")
                        {
                            return;
                        }
                        ContentBrowserWidget::FocusAssetPath(textureRef);
                    };

                    ImGui::AssetInputSlot(diffuseRef, "Texture", [&]() { Dirty = true; }, "Diffuse", [&]() { focusTextureAsset(diffuseRef); });
                    ImGui::AssetInputSlot(normalRef, "Texture", [&]() { Dirty = true; }, "Normal", [&]() { focusTextureAsset(normalRef); });
                    ImGui::AssetInputSlot(ormRef, "Texture", [&]() { Dirty = true; }, "ORM", [&]() { focusTextureAsset(ormRef); });

                    ImGui::Spacing();
                    ImGui::TextUnformatted("Surface");

                    float albedo[4] = { LoadedMaterial->Albedo.x, LoadedMaterial->Albedo.y, LoadedMaterial->Albedo.z, LoadedMaterial->Albedo.w };
                    if (ImGui::ColorEdit4("Albedo", albedo))
                    {
                        LoadedMaterial->Albedo = Vector4(albedo[0], albedo[1], albedo[2], albedo[3]);
                        Dirty = true;
                    }

                    if (ImGui::SliderFloat("Metallic", &LoadedMaterial->Metallic, 0.0f, 1.0f))
                    {
                        Dirty = true;
                    }

                    if (ImGui::SliderFloat("Roughness", &LoadedMaterial->Roughness, 0.0f, 1.0f))
                    {
                        Dirty = true;
                    }

                    if(ImGui::Checkbox("AlphaCut", &LoadedMaterial->AlphaCut))
                    {
                        Dirty = true;
                    }

                    if(ImGui::Checkbox("Cast Shadows", &LoadedMaterial->CastShadows))
                    {
                        Dirty = true;
                    }

                    ImGui::Spacing();
                    if (Dirty)
                    {
                        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "Unsaved changes");
                    }

                    if (ImGui::Button("Save Material"))
                    {
                        SaveLoadedMaterial();
                    }
                }
            }

            ImGui::End();
        }
    };
}
