#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "imgui.h"
#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/ECS/Components/AudioListener.h"
#include "Waldem/ECS/Systems/UISystems/EditorControlSystem.h"
#include "Waldem/ECS/Systems/UISystems/EditorGuizmoSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/FreeLookCameraSystem.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Systems/UISystems/EditorUISystem.h"
#include "Waldem/ECS/Systems/DrawSystems/OceanSimulationSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/ScreenQuadSystem.h"
#include "Waldem/ECS/Systems/UpdateSystems/SpatialAudioSystem.h"
#include "Waldem/Events/ApplicationEvent.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Utils/FileUtils.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Systems/DrawSystems/AnimationSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/HybridRenderingSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/PostProcessSystem.h"
#include "Waldem/ECS/Systems/DrawSystems/SkyRenderingSystem.h"

namespace Waldem
{
    class WALDEM_API EditorLayer : public Layer
    {
    private:
        CWindow* Window;
        bool BlockUIEvents = true;
        bool ImportSceneThisFrame = false;
        bool EditorViewportResizeTriggered = false;
        Vector2 NewEditorViewportSize = {};
        
    public:
        EditorLayer(CWindow* window) : Layer("EditorLayer", window)
        {
            InputManager = {};
            
            Vector2 size = { 1920, 1080 };
            Renderer::CreateRenderTarget("TargetRT", size.x, size.y, TextureFormat::R8G8B8A8_UNORM);
            Renderer::CreateRenderTarget("WorldPositionRT", size.x, size.y, TextureFormat::R32G32B32A32_FLOAT);
            Renderer::CreateRenderTarget("NormalRT", size.x, size.y, TextureFormat::R16G16B16A16_FLOAT);
            Renderer::CreateRenderTarget("ColorRT", size.x, size.y, TextureFormat::R8G8B8A8_UNORM);
            Renderer::CreateRenderTarget("ORMRT", size.x, size.y, TextureFormat::R32G32B32A32_FLOAT);
            Renderer::CreateRenderTarget("MeshIDRT", size.x, size.y, TextureFormat::R32_SINT);
            Renderer::CreateRenderTarget("DepthRT", size.x, size.y, TextureFormat::D32_FLOAT);
            Renderer::CreateRenderTarget("RadianceRT", size.x, size.y, TextureFormat::R32G32B32A32_FLOAT);
            Renderer::CreateRenderTarget("ReflectionRT", size.x, size.y, TextureFormat::R32G32B32A32_FLOAT);
            
            auto cameraEntity = ECS::CreateEntity("EditorCamera");
            float aspectRatio = size.x / size.y;
            cameraEntity.set<Transform>({Vector3(0, 10, -10.f)});
            cameraEntity.set<Camera>({60.0f, aspectRatio, 0.001f, 1000.0f, 30.0f, 30.0f});
            cameraEntity.add<AudioListener>();

            auto skyEntity = ECS::CreateEntity("Sky");
            skyEntity.add<Transform>();
            skyEntity.add<Sky>();
            
            //do it after all entities set up
            UISystems.Add(new EditorUISystem());
            UISystems.Add(new EditorGuizmoSystem());
            
            UpdateSystems.Add(new EditorControlSystem());
            UpdateSystems.Add(new SpatialAudioSystem());

            // DrawSystems.Add(new OceanSimulationSystem());
            DrawSystems.Add(new AnimationSystem());
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
            Renderer::ResizeRenderTarget("TargetRT", size.x, size.y);
            Renderer::ResizeRenderTarget("WorldPositionRT", size.x, size.y);
            Renderer::ResizeRenderTarget("NormalRT", size.x, size.y);
            Renderer::ResizeRenderTarget("ColorRT", size.x, size.y);
            Renderer::ResizeRenderTarget("ORMRT", size.x, size.y);
            Renderer::ResizeRenderTarget("MeshIDRT", size.x, size.y);
            Renderer::ResizeRenderTarget("DepthRT", size.x, size.y);
            Renderer::ResizeRenderTarget("RadianceRT", size.x, size.y);
            
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
                    event.Handled = CContentManager::Broadcast(event);
                    break;
                }
            }
        }
    };
}
