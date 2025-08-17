#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <fstream>
#include <filesystem>
#include "imgui.h"
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Systems/UISystems/EditorControlSystem.h"
#include "Waldem/ECS/Systems/UISystems/EditorGuizmoSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/EditorComponent.h"
#include "Waldem/ECS/Systems/UISystems/EditorUISystem.h"
#include "Waldem/ECS/Systems/DrawSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/SpatialAudioSystem.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Events/FileEvent.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/SceneManagement/GameScene.h"
#include "Waldem/Utils/FileUtils.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Systems/DrawSystems/HybridRenderingSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/PostProcessSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/SkyRenderingSystem.h"

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
        EditorLayer(CWindow* window, ResourceManager* resourceManager) : Layer("EditorLayer", window, resourceManager)
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
            
            auto cameraEntity = ECS::CreateEntity("EditorCamera");
            float aspectRatio = size.x / size.y;
            cameraEntity.set<Transform>({Vector3(0, 10, -10.f)});
            cameraEntity.set<Camera>({60.0f, aspectRatio, 0.001f, 1000.0f, 30.0f, 30.0f});
            cameraEntity.add<AudioListener>();

            auto skyEntity = ECS::CreateEntity("Sky");
            skyEntity.add<Transform>();
            skyEntity.add<Sky>();
            
            //do it after all entities set up
            UISystems.Add(new EditorUISystem(BIND_ACTION(OnOpenScene), BIND_ACTION(OnSaveScene), BIND_ACTION(OnSaveSceneAs)));
            UISystems.Add(new EditorGuizmoSystem());
            
            UpdateSystems.Add(new EditorControlSystem());
            UpdateSystems.Add(new SpatialAudioSystem());

            // DrawSystems.Add(new OceanSimulationSystem());
            DrawSystems.Add(new HybridRenderingSystem());
            DrawSystems.Add(new PostProcessSystem());
            DrawSystems.Add(new ScreenQuadSystem());

            Window = window;

            Renderer::GetEditorViewport()->SubscribeOnResize([this](Vector2 size)
            {
                OnResize(size);
            });

            ECS::World.system<>("UISystems").kind(flecs::OnGUI).each([&]
            {
                for (auto system : UISystems)
                {
                    system->Update(ECS::World.delta_time());
                }
            });
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
            
            for (ISystem* system : DrawSystems)
            {
                system->OnResize(size);
            }

            ECS::World.query<Camera>().each([size](flecs::entity entity, Camera& camera)
            {
                camera.UpdateProjectionMatrix(camera.FieldOfView, size.x/size.y, camera.NearPlane, camera.FarPlane);
                entity.modified<Camera>(); 
            });
        }
        
        void OnEvent(Event& event) override
        {
            auto editorViewport = Renderer::GetEditorViewport();
            auto eventType = event.GetEventType();

            if (BlockUIEvents)
            {
                event.Handled |= event.IsInCategory(EventCategoryKeyboard) & ImGui::GetIO().WantCaptureKeyboard;
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
                {
                    event.Handled |= event.IsInCategory(EventCategoryMouse) & (ImGui::GetIO().WantCaptureMouse && !editorViewport->IsMouseOver);
                }
            case EventType::MouseButtonReleased:
                {
                    if(event.Handled)
                    {
                        break;
                    }
                    
                    event.Handled = InputManager.Broadcast(event);
                    break;
                }
            case EventType::FileDropped:
                {
                    event.Handled = ContentManager.Broadcast(event);
                    break;
                }
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

        void LoadScene(Path& path)
        {
            //create new scene
            CurrentScene = new GameScene();
            CurrentScene->Initialize(&InputManager, CurrentResourceManager);

            //deserialize input data into the scene
            CurrentScene->Deserialize(path);
        }

        void OpenScene(GameScene* scene)
        {
            // Deinitialize();
            
            CloseScene(CurrentScene);
            
            CurrentScene = scene;
            CurrentScene->Initialize(&InputManager, CurrentResourceManager);

            // Initialize();
        }

        void CheckImportSceneThisFrame()
        {
            if(ImportSceneThisFrame)
            {
                ImportScene(CurrentScenePath);
                ImportSceneThisFrame = false;
            }
        }

        void ImportScene(Path& path)
        {
            // Deinitialize();
            
            CloseScene(CurrentScene);

            LoadScene(path);
            // Initialize();
        }

        void ExportScene(Path& path)
        {
            CurrentScene->Serialize(path);
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
