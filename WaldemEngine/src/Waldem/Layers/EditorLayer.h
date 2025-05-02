#pragma once

#include <fstream>
#include <filesystem>
#include "imgui.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Systems/EditorSystems/EditorControlSystem.h"
#include "..\ECS\Systems\EditorSystems\EditorGuizmoSystem.h"
#include "Waldem/ECS/Systems/GameSystems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Systems/EditorSystems/EditorUISystem.h"
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
        Path CurrentScenePath;
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

            UISystems.Add(new EditorUISystem(CurrentECSManager, BIND_ACTION(OnOpenScene), BIND_ACTION(OnSaveScene), BIND_ACTION(OnSaveSceneAs)));
            UISystems.Add(new EditorGuizmoSystem(CurrentECSManager));
            
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
        }
        
        void OnDetach() override
        {
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

                    SceneData sceneData = { Window };
                    
                    LoadScene(inData, sceneData);

                    Initialize(&sceneData);
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
