#pragma once

#include <fstream>
#include <filesystem>
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl2.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Systems/EditorSystems/EditorControlSystem.h"
#include "Waldem/ECS/Systems/EditorSystems/GuizmoEditorSystem.h"
#include "Waldem/ECS/Systems/GameSystems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/EntityDetailsWidgetContainer.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/MainWidgetContainer.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/HierarchyWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/BloomComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/ColliderComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/LightComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/NameWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/OceanComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/TransformComponentWidget.h"
#include "Waldem/ECS/Systems/GameSystems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/GameSystems/GBufferSystem.h"
#include "Waldem/ECS/Systems/GameSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/GameSystems/RayTracingRadianceSystem.h"
#include "Waldem/ECS/Systems/GameSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/GameSystems/SpatialAudioSystem.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/SceneManagement/GameScene.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    class WALDEM_API EditorLayer : public Layer
    {
    private:
        CWindow* Window;
        GameScene* CurrentScene = nullptr;
        bool BlockUIEvents = true;
        std::filesystem::path CurrentScenePath;
        bool ImportSceneThisFrame = false;
        
    public:
        EditorLayer(CWindow* window, ECSManager* ecsManager, ResourceManager* resourceManager) : Layer("EditorLayer", window, ecsManager, resourceManager)
        {
            InputManager = {};
            
            auto cameraEntity = CurrentECSManager->CreateEntity("Camera");
            float aspectRatio = window->GetWidth() / window->GetHeight();
            cameraEntity->Add<Transform>(Vector3(0, 0, 0));
            cameraEntity->Add<Camera>(60.0f, aspectRatio, 0.001f, 1000.0f, 30.0f, 30.0f);
            cameraEntity->Add<EditorCamera>();
            cameraEntity->Add<AudioListener>();
            
            //do it after all entities set up
            CurrentECSManager->Refresh();

            UISystems.Add(CreateMainWidget());
            UISystems.Add(new GuizmoEditorSystem(CurrentECSManager));
            
            UpdateSystems.Add(new EditorControlSystem(CurrentECSManager));
            UpdateSystems.Add(new SpatialAudioSystem(ecsManager));

            DrawSystems.Add(new OceanSimulationSystem(ecsManager));
            DrawSystems.Add(new GBufferSystem(ecsManager));
            DrawSystems.Add(new RayTracingRadianceSystem(ecsManager));
            DrawSystems.Add(new DeferredRenderingSystem(ecsManager));
            // DrawSystems.Add(new PostProcessSystem(ecsManager));
            DrawSystems.Add(new ScreenQuadSystem(ecsManager));

            Window = window;
        }

        MainWidgetContainer* CreateMainWidget()
        {
            return new MainWidgetContainer(CurrentECSManager,
            {
                new HierarchyWidget(CurrentECSManager),
                new EntityDetailsWidgetContainer(CurrentECSManager,
                {
                    //put all component widgets here
                    new NameWidget(CurrentECSManager),
                    new TransformComponentWidget(CurrentECSManager),
                    new ColliderComponentWidget(CurrentECSManager),
                    new LightComponentWidget(CurrentECSManager),
                    new BloomComponentWidget(CurrentECSManager),
                    new OceanComponentWidget(CurrentECSManager),
                }),
            });
        }
        
        void Begin() override
        {
            Renderer::BeginUI();
        }
        
        void End() override
        {
            Renderer::EndUI();
        }
        
        void OnAttach() override
        {
            InitializeUI();
        }
        
        void OnDetach() override
        {
            DeinitializeUI();
        }
        
        void OnEvent(Event& event) override
        {
            auto eventType = event.GetEventType();

            if (BlockUIEvents)
            {
                ImGuiIO& io = ImGui::GetIO();
                event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
                event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
                event.Handled |= ImGuizmo::IsUsing();
            }
            
            switch (eventType)
            {
            case EventType::MouseMoved:
                {
                    InputManager.Broadcast(event);
                    break;
                }
            case EventType::MouseScrolled:
            case EventType::KeyPressed:
            case EventType::KeyReleased:
            case EventType::KeyTyped:
                {
                    event.Handled = InputManager.Broadcast(event);
                    break;
                }
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
                {
                    if(event.Handled) //for these events we dont want to broadcast them if they are already handled by UI handlers to avoid counter-behaving
                    {
                        break;
                    }
                    
                    event.Handled = InputManager.Broadcast(event);
                    break;
                }
            }
        }

        void OnUpdate(float deltaTime) override
        {
            if(ImportSceneThisFrame)
            {
                ImportScene(CurrentScenePath);
                ImportSceneThisFrame = false;
            }

            for (ISystem* system : UpdateSystems)
            {
                system->Update(deltaTime);
            }
        }

        void OnDraw(float deltaTime) override
        {
            for (ISystem* system : DrawSystems)
            {
                system->Update(deltaTime);
            }
        }

        void OnDrawUI(float deltaTime) override
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New scene")) {}
                    if (ImGui::MenuItem("Open scene", "Ctrl+O"))
                    {
                        if(OpenFile(CurrentScenePath))
                        {
                            ImportSceneThisFrame = true;
                        }
                    }
                    if (ImGui::MenuItem("Save scene"))
                    {
                        bool save = true;
                        if(CurrentScenePath.empty())
                        {
                            save = SaveFile(CurrentScenePath);
                        }

                        if(save)
                        {
                            ExportScene(CurrentScenePath);
                        }
                    }
                    if (ImGui::MenuItem("Save scene as..."))
                    {
                        if(SaveFile(CurrentScenePath))
                        {
                            ExportScene(CurrentScenePath);
                        }
                    }
            
                    ImGui::Separator();
					       
                    if (ImGui::BeginMenu("Options"))
                    {
                        ImGui::EndMenu();
                    }
					       
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            for (ISystem* system : UISystems)
            {
                system->Update(deltaTime);
            }
        }

        void InitializeUI()
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
            // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable docking
            // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable multi-viewport

            SDL_Window* window = static_cast<SDL_Window*>(CWindow::GetNativeWindow());
            ImGui_ImplSDL2_InitForD3D(window);

            // Style
            ImGui::StyleColorsDark();

            Renderer::InitializeUI();
        }

        void DeinitializeUI()
        {
            ImGui_ImplSDL2_Shutdown();
            ImGui_ImplDX12_Shutdown();
            ImGui::DestroyContext();
        }

        void CloseScene(GameScene* scene)
        {                    
            //destroy previous scene
            if(scene)
            {
                delete scene;
                scene = nullptr;
            }
        }

        void LoadScene(WDataBuffer& dataBuffer, SceneData& sceneData)
        {
            //create new scene
            CurrentScene = new GameScene();
            CurrentScene->Initialize(&sceneData, &InputManager, CurrentECSManager, CurrentResourceManager);

            //deserialize input data into the scene
            CurrentScene->Deserialize(dataBuffer);
        }

        void OpenScene(GameScene* scene, SceneData* sceneData)
        {
            Deinitialize();
            
            CloseScene(CurrentScene);
            
            CurrentScene = scene;
            CurrentScene->Initialize(sceneData, &InputManager, CurrentECSManager, CurrentResourceManager);

            Initialize(sceneData);
        }

        void ImportScene(std::filesystem::path path)
        {
            std::ifstream inFile(path.c_str(), std::ios::binary | std::ios::ate);
            if (inFile.is_open())
            {
                std::streamsize size = inFile.tellg();
                inFile.seekg(0, std::ios::beg);

                unsigned char* buffer = new unsigned char[size];
                if (inFile.read((char*)buffer, size))
                {
                    Deinitialize();
                    
                    CloseScene(CurrentScene);

                    WDataBuffer inData = WDataBuffer(buffer, size);

                    SceneData sceneData = { Window };
                    
                    LoadScene(inData, sceneData);

                    Initialize(&sceneData);
                }
                delete[] buffer;
                inFile.close();
            }
        }

        void ExportScene(std::filesystem::path path)
        {
            WDataBuffer outData;
            CurrentScene->Serialize(outData);
            
            std::ofstream outFile(path.c_str(), std::ios::binary);
            outFile.write(static_cast<const char*>(outData.GetData()), outData.GetSize());
            outFile.close();
        }
    };
}
