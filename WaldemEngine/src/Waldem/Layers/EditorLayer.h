#pragma once

#include <fstream>
#include <filesystem>
#include "imgui.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Systems/UISystems/EditorControlSystem.h"
#include "Waldem/ECS/Systems/UISystems/EditorGuizmoSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Systems/UISystems/EditorUISystem.h"
#include "Waldem/ECS/Systems/DrawSystems/DeferredRenderingSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/GBufferSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/RayTracingRadianceSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/SpatialAudioSystem.h"
#include "Waldem/Events/ApplicationEvent.h"
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
        Path CurrentScenePath;
        bool ImportSceneThisFrame = false;
        bool EditorViewportResizeTriggered = false;
        Vector2 NewEditorViewportSize = {};
        
    public:
        EditorLayer(CWindow* window, ECSManager* ecsManager, ResourceManager* resourceManager) : Layer("EditorLayer", window, ecsManager, resourceManager)
        {
            InputManager = {};
            
            Vector2 size = { 1920, 1080 };
            resourceManager->CreateRenderTarget("TargetRT", size.x, size.y, TextureFormat::R8G8B8A8_UNORM);
            resourceManager->CreateRenderTarget("WorldPositionRT", size.x, size.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("NormalRT", size.x, size.y, TextureFormat::R16G16B16A16_FLOAT);
            resourceManager->CreateRenderTarget("ColorRT", size.x, size.y, TextureFormat::R8G8B8A8_UNORM);
            resourceManager->CreateRenderTarget("ORMRT", size.x, size.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("MeshIDRT", size.x, size.y, TextureFormat::R32_SINT);
            resourceManager->CreateRenderTarget("DepthRT", size.x, size.y, TextureFormat::D32_FLOAT);
            resourceManager->CreateRenderTarget("RadianceRT", size.x, size.y, TextureFormat::R32G32B32A32_FLOAT);
            
            auto cameraEntity = CurrentECSManager->CreateEntity("Camera");
            float aspectRatio = size.x / size.y;
            cameraEntity->Add<Transform>(Vector3(0, 0, 0));
            cameraEntity->Add<Camera>(60.0f, aspectRatio, 0.001f, 1000.0f, 30.0f, 30.0f);
            cameraEntity->Add<EditorCamera>();
            cameraEntity->Add<AudioListener>();
            
            //do it after all entities set up
            CurrentECSManager->Refresh();

            UISystems.Add(new EditorUISystem(CurrentECSManager, BIND_ACTION(OnOpenScene), BIND_ACTION(OnSaveScene), BIND_ACTION(OnSaveSceneAs)));
            UISystems.Add(new EditorGuizmoSystem(CurrentECSManager));
            
            UpdateSystems.Add(new EditorControlSystem(CurrentECSManager));
            UpdateSystems.Add(new SpatialAudioSystem(ecsManager));

            // DrawSystems.Add(new OceanSimulationSystem(ecsManager));
            DrawSystems.Add(new GBufferSystem(ecsManager));
            DrawSystems.Add(new RayTracingRadianceSystem(ecsManager));
            DrawSystems.Add(new DeferredRenderingSystem(ecsManager));
            // DrawSystems.Add(new PostProcessSystem(ecsManager));
            DrawSystems.Add(new ScreenQuadSystem(ecsManager));

            Window = window;

            Editor::SubscribeOnResize([this](Vector2 size)
            {
                EditorViewportResizeTriggered = true;
                NewEditorViewportSize = size;
            });
        }
        
        void Begin() override
        {
        }
        
        void End() override
        {
        }
        
        void OnAttach() override
        {
        }
        
        void OnDetach() override
        {
        }

        void OnResize(Vector2 size)
        {
            CurrentResourceManager->ResizeRenderTarget("TargetRT", size.x, size.y);
            CurrentResourceManager->ResizeRenderTarget("WorldPositionRT", size.x, size.y);
            CurrentResourceManager->ResizeRenderTarget("NormalRT", size.x, size.y);
            CurrentResourceManager->ResizeRenderTarget("ColorRT", size.x, size.y);
            CurrentResourceManager->ResizeRenderTarget("ORMRT", size.x, size.y);
            CurrentResourceManager->ResizeRenderTarget("MeshIDRT", size.x, size.y);
            CurrentResourceManager->ResizeRenderTarget("DepthRT", size.x, size.y);
            CurrentResourceManager->ResizeRenderTarget("RadianceRT", size.x, size.y);
            
            for (DrawSystem* system : DrawSystems)
            {
                system->OnResize(size);

                //TODO: this is a temporary solution, replace with only OnResize call
                system->Deinitialize();
                system->Initialize(&InputManager, CurrentResourceManager);
            }
            
            for (auto [entity, camera] : CurrentECSManager->EntitiesWith<Camera>())
            {
                camera.UpdateProjectionMatrix(camera.FieldOfView, size.x/size.y, camera.NearPlane, camera.FarPlane); 
            }
        }
        
        void OnEvent(Event& event) override
        {
            auto eventType = event.GetEventType();

            if (BlockUIEvents)
            {
                ImGuiIO& io = ImGui::GetIO();
                bool shouldBlockMouse = io.WantCaptureMouse && !Editor::IsMouseOverEditorViewport;
                bool shouldBlockKeyboard = io.WantCaptureKeyboard && !Editor::IsMouseOverEditorViewport;
                
                event.Handled |= event.IsInCategory(EventCategoryMouse) & shouldBlockMouse;
                event.Handled |= event.IsInCategory(EventCategoryKeyboard) & shouldBlockKeyboard;
                event.Handled |= Editor::GizmoIsUsing;
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
            
            if(EditorViewportResizeTriggered)
            {
                OnResize(NewEditorViewportSize);
                EditorViewportResizeTriggered = false;
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
            for (ISystem* system : UISystems)
            {
                system->Update(deltaTime);
            }
        }

        void CloseScene(GameScene* scene)
        {                    
            if(scene)
            {
                delete scene;
                scene = nullptr;
            }
        }

        void LoadScene(WDataBuffer& dataBuffer)
        {
            //create new scene
            CurrentScene = new GameScene();
            CurrentScene->Initialize(&InputManager, CurrentECSManager, CurrentResourceManager);

            //deserialize input data into the scene
            CurrentScene->Deserialize(dataBuffer);
        }

        void OpenScene(GameScene* scene)
        {
            Deinitialize();
            
            CloseScene(CurrentScene);
            
            CurrentScene = scene;
            CurrentScene->Initialize(&InputManager, CurrentECSManager, CurrentResourceManager);

            Initialize();
        }

        void ImportScene(Path path)
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

                    LoadScene(inData);

                    Initialize();
                }
                delete[] buffer;
                inFile.close();
            }
        }

        void ExportScene(Path path)
        {
            WDataBuffer outData;
            CurrentScene->Serialize(outData);
            
            std::ofstream outFile(path.c_str(), std::ios::binary);
            outFile.write(static_cast<const char*>(outData.GetData()), outData.GetSize());
            outFile.close();
        }

        void OnOpenScene()
        {
            if(OpenFile(CurrentScenePath))
            {
                ImportSceneThisFrame = true;
            }
        }

        void OnSaveScene()
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

        void OnSaveSceneAs()
        {
            if(SaveFile(CurrentScenePath))
            {
                ExportScene(CurrentScenePath);
            }
        }
    };
}
