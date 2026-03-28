#pragma once
#include "imgui.h"
#include "Widget.h"
#include "Waldem/Coach/TinyCuda/NIV/NIVCoach.h"
#include "Waldem/ECS/Systems/CoreSystems/TrainingPathTracingSystem.h"
#include "Waldem/Renderer/Renderer.h"
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

namespace Waldem
{
    class CoachWidget : public IWidget
    {
    private:
        inline static bool Visible = false;

        struct CoachParams
        {
            uint32 InputDims = 0;
            uint32 OutputDims = 0;
            uint32 BatchSize = 0;
            uint32 TrainingSteps = 0;
            std::string DatasetPath;
            std::string CheckpointPath;
        };

        std::unique_ptr<Coach::TinyCuda::NIVCoach> NIVCoachInstance;
        std::unique_ptr<LiveNIVTrainingDataProvider> LiveTrainingProvider;
        int InputDims = 5;
        int OutputDims = 3;
        int BatchSize = 32768;
        int TrainingSteps = 10000;
        char DatasetPath[512] = "Content/Training/irradiance_samples.bin";
        char CheckpointPath[512] = "Content/Training/niv_model.ckpt";
        bool CoachInitialized = false;
        CoachParams LastInitializedParams;
        std::string StatusMessage = "Not initialized";
        bool LastActionFailed = false;
        float InferPos[3] = { 0.0f, 0.0f, 0.0f };
        float InferNormal[3] = { 0.0f, 1.0f, 0.0f };
        float InferResult[3] = { 0.0f, 0.0f, 0.0f };

        CoachParams GetCurrentParams() const
        {
            CoachParams params{};
            params.InputDims = InputDims > 1 ? (uint32)InputDims : 1u;
            params.OutputDims = OutputDims > 1 ? (uint32)OutputDims : 1u;
            params.BatchSize = BatchSize > 1 ? (uint32)BatchSize : 1u;
            params.TrainingSteps = TrainingSteps > 1 ? (uint32)TrainingSteps : 1u;
            params.DatasetPath = std::string(DatasetPath);
            params.CheckpointPath = std::string(CheckpointPath);
            return params;
        }

        bool ParametersChanged(const CoachParams& params) const
        {
            return !CoachInitialized
                || params.InputDims != LastInitializedParams.InputDims
                || params.OutputDims != LastInitializedParams.OutputDims
                || params.BatchSize != LastInitializedParams.BatchSize
                || params.TrainingSteps != LastInitializedParams.TrainingSteps
                || params.DatasetPath != LastInitializedParams.DatasetPath
                || params.CheckpointPath != LastInitializedParams.CheckpointPath;
        }

        std::filesystem::path ResolveProjectContentRoot() const
        {
            std::error_code ec;
            std::filesystem::path current = std::filesystem::current_path(ec);
            if (ec)
            {
                current.clear();
            }

            auto isProjectRoot = [](const std::filesystem::path& candidate) -> bool
            {
                std::error_code localEc;
                return std::filesystem::exists(candidate / "Content", localEc) &&
                       std::filesystem::exists(candidate / "Source", localEc);
            };

            for (std::filesystem::path probe = current; !probe.empty(); probe = probe.parent_path())
            {
                if (isProjectRoot(probe))
                {
                    return (probe / "Content").lexically_normal();
                }

                if (probe == probe.root_path())
                {
                    break;
                }
            }

            return std::filesystem::absolute(std::filesystem::path(CONTENT_PATH)).lexically_normal();
        }

        std::filesystem::path ResolveContentPath(const std::string& path) const
        {
            if (path.empty())
            {
                return {};
            }

            std::filesystem::path p(path);
            if (p.is_absolute())
            {
                return p.lexically_normal();
            }

            std::filesystem::path contentRoot = ResolveProjectContentRoot();
            const std::string generic = p.generic_string();
            if (generic.rfind("Content/", 0) == 0 || generic == "Content")
            {
                return (contentRoot.parent_path() / p).lexically_normal();
            }

            return (contentRoot / "Training" / p.filename()).lexically_normal();
        }

        bool CheckpointExists(const std::string& checkpointPath) const
        {
            const std::filesystem::path resolved = ResolveContentPath(checkpointPath);
            return !resolved.empty() && std::filesystem::exists(resolved);
        }

        std::string BuildUniqueCheckpointPath() const
        {
            std::filesystem::path absoluteBase = ResolveContentPath(std::string(CheckpointPath));
            if (absoluteBase.empty())
            {
                absoluteBase = ResolveContentPath("Content/Training/niv_model.ckpt");
            }

            std::filesystem::create_directories(absoluteBase.parent_path());

            std::filesystem::path chosen = absoluteBase;
            for (uint32 suffix = 1; std::filesystem::exists(chosen); ++suffix)
            {
                chosen = absoluteBase.parent_path() /
                    std::filesystem::path(absoluteBase.stem().string() + "_" + std::to_string(suffix) + absoluteBase.extension().string());
            }

            std::error_code ec;
            const std::filesystem::path contentRoot = ResolveProjectContentRoot();
            std::filesystem::path relative = std::filesystem::relative(chosen, contentRoot.parent_path(), ec);
            if (!ec)
            {
                return relative.generic_string();
            }

            return chosen.generic_string();
        }

        void InitializeCoach(bool forceFresh = false)
        {
            try
            {
                if (forceFresh)
                {
                    NIVCoachInstance.reset();
                    CoachInitialized = false;
                }

                if (!NIVCoachInstance)
                {
                    NIVCoachInstance = std::make_unique<Coach::TinyCuda::NIVCoach>();
                }
                if (!LiveTrainingProvider)
                {
                    LiveTrainingProvider = std::make_unique<LiveNIVTrainingDataProvider>();
                }

                const CoachParams params = GetCurrentParams();
                NIVCoachInstance->SetTrainingDataProvider(LiveTrainingProvider.get());
                NIVCoachInstance->SetCheckpointPath(params.CheckpointPath);
                NIVCoachInstance->SetAutoCheckpointInterval(1000);

                NIVCoachInstance->Initialize(
                    params.InputDims,
                    params.OutputDims,
                    params.BatchSize,
                    params.TrainingSteps,
                    params.DatasetPath
                );

                CoachInitialized = true;
                LastInitializedParams = params;
                Renderer::RenderData.NIVRuntimeCoach = NIVCoachInstance.get();
                LastActionFailed = false;
                StatusMessage = "Coach initialized";
            }
            catch (const std::exception& e)
            {
                CoachInitialized = false;
                Renderer::RenderData.NIVRuntimeCoach = nullptr;
                LastActionFailed = true;
                StatusMessage = std::string("Initialize failed: ") + e.what();
            }
            catch (...)
            {
                CoachInitialized = false;
                Renderer::RenderData.NIVRuntimeCoach = nullptr;
                LastActionFailed = true;
                StatusMessage = "Initialize failed: unknown error";
            }
        }

        bool EnsureInitializedForCurrentParams(bool forceFresh = false)
        {
            const CoachParams params = GetCurrentParams();
            if (forceFresh || ParametersChanged(params))
            {
                InitializeCoach(forceFresh);
            }
            return CoachInitialized && NIVCoachInstance != nullptr;
        }

        void StartTraining(bool continueTraining)
        {
            try
            {
                if (!continueTraining)
                {
                    const std::string freshCheckpointPath = BuildUniqueCheckpointPath();
                    std::strncpy(CheckpointPath, freshCheckpointPath.c_str(), sizeof(CheckpointPath) - 1);
                    CheckpointPath[sizeof(CheckpointPath) - 1] = '\0';
                }
                else if (!CheckpointExists(std::string(CheckpointPath)))
                {
                    LastActionFailed = true;
                    StatusMessage = "Continue training failed: checkpoint file does not exist";
                    return;
                }

                if (EnsureInitializedForCurrentParams(!continueTraining))
                {
                    NIVCoachInstance->Train();
                    LastActionFailed = false;
                    StatusMessage = continueTraining ? "Continue training finished" : "New training finished";
                }
            }
            catch (const std::exception& e)
            {
                LastActionFailed = true;
                StatusMessage = std::string(continueTraining ? "Continue training failed: " : "New training failed: ") + e.what();
            }
            catch (...)
            {
                LastActionFailed = true;
                StatusMessage = continueTraining ? "Continue training failed: unknown error" : "New training failed: unknown error";
            }
        }

    public:
        CoachWidget() = default;

        static bool IsVisible()
        {
            return Visible;
        }

        static void SetVisible(bool visible)
        {
            Visible = visible;
        }

        void Initialize(InputManager* inputManager) override
        {
            (void)inputManager;

            auto& renderData = Renderer::RenderData;
            const char* path = renderData.TrainingDatasetOutputPath.C_Str();
            if (path && path[0] != '\0')
            {
                std::strncpy(DatasetPath, path, sizeof(DatasetPath) - 1);
                DatasetPath[sizeof(DatasetPath) - 1] = '\0';
            }

            InitializeCoach();
        }

        void OnDraw(float deltaTime) override
        {
            (void)deltaTime;
            if(!Visible)
            {
                return;
            }

            const bool isVisible = ImGui::Begin("Coach###Coach", &Visible);
            if (!isVisible)
            {
                ImGui::End();
                return;
            }

            auto& renderData = Renderer::RenderData;

            ImGui::SeparatorText("Training");
            {
                int trainingRaysPerPoint = (int)renderData.TrainingDatasetRaysPerPoint;
                int trainingBounces = (int)renderData.TrainingDatasetMaxBounces;
                int trainingSeed = (int)renderData.TrainingDatasetSeed;

                if (ImGui::DragInt("Rays/Point", &trainingRaysPerPoint, 1.0f, 1, 1024))
                {
                    renderData.TrainingDatasetRaysPerPoint = trainingRaysPerPoint > 1 ? (uint)trainingRaysPerPoint : 1u;
                }

                if (ImGui::DragInt("Bounces", &trainingBounces, 0.25f, 1, 6))
                {
                    renderData.TrainingDatasetMaxBounces = trainingBounces > 1 ? (uint)trainingBounces : 1u;
                }

                if (ImGui::InputInt("Seed", &trainingSeed))
                {
                    renderData.TrainingDatasetSeed = trainingSeed > 0 ? (uint)trainingSeed : 1;
                }

                ImGui::TextWrapped("Live training samples fresh path-traced batches from the current scene.");
                ImGui::InputInt("Input Dims", &InputDims);
                ImGui::InputInt("Output Dims", &OutputDims);
            }

            ImGui::InputInt("Samples/Step", &BatchSize);
            ImGui::InputInt("Training Steps", &TrainingSteps);
            ImGui::InputText("Checkpoint Path", CheckpointPath, IM_ARRAYSIZE(CheckpointPath));
            ImGui::TextDisabled("Samples/Step is the live-training batch size.");

            if (ImGui::Button("New Training"))
            {
                StartTraining(false);
            }

            ImGui::SameLine();
            if (ImGui::Button("Continue Training"))
            {
                StartTraining(true);
            }

            if (LastActionFailed)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", StatusMessage.c_str());
            }
            else
            {
                ImGui::Text("%s", StatusMessage.c_str());
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Inference");

            ImGui::InputFloat3("Infer Position", InferPos);
            ImGui::InputFloat3("Infer Normal", InferNormal);

            if (ImGui::Button("Infer Irradiance"))
            {
                if (EnsureInitializedForCurrentParams())
                {
                    try
                    {
                        NIVCoachInstance->InferIrradiance(
                            InferPos[0], InferPos[1], InferPos[2],
                            InferNormal[0], InferNormal[1], InferNormal[2],
                            InferResult[0], InferResult[1], InferResult[2]
                        );
                        renderData.FeatureToggles.EnableNIVInference = true;
                        LastActionFailed = false;
                        StatusMessage = "Inference finished";
                    }
                    catch (const std::exception& e)
                    {
                        LastActionFailed = true;
                        StatusMessage = std::string("Inference failed: ") + e.what();
                    }
                    catch (...)
                    {
                        LastActionFailed = true;
                        StatusMessage = "Inference failed: unknown error"; 
                    }
                }
            }

            ImGui::Text("Predicted Irradiance RGB: %.6f, %.6f, %.6f", InferResult[0], InferResult[1], InferResult[2]);
            ImGui::Checkbox("Temporal Smoothing##NIV", &renderData.EnableNIVTemporalSmoothing);
            ImGui::SliderFloat("History Weight##NIV", &renderData.NIVTemporalHistoryWeight, 0.0f, 0.98f, "%.2f");
            ImGui::Checkbox("Spatial Filter##NIV", &renderData.EnableNIVSpatialFilter);
            ImGui::SliderFloat("Filter Strength##NIV", &renderData.NIVSpatialFilterStrength, 0.0f, 1.0f, "%.2f");
            ImGui::Text("NIV Runtime: attempted=%s succeeded=%s validPixels=%u meanLum=%.6f max=%.6f",
                renderData.NIVLastInferenceAttempted ? "true" : "false",
                renderData.NIVLastInferenceSucceeded ? "true" : "false",
                renderData.NIVLastValidPixelCount,
                renderData.NIVLastMeanLuminance,
                renderData.NIVLastMaxChannel);

            ImGui::End();
        }
    };
}
