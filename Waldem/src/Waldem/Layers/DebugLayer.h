#pragma once

#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/DebugSystems/CollisionRenderingSystem.h"
#include "Waldem/ECS/Systems/DebugSystems/DebugSystem.h"
#include "Waldem/ECS/Systems/DebugSystems/LinesRenderingSystem.h"
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"

namespace Waldem
{
    class WALDEM_API DebugLayer : public Layer
    {
    private:
        bool EnableDebug = false;
        
    public:
        DebugLayer(Window* window, ecs::Manager* ecsManager, ResourceManager* resourceManager) : Layer("DebugLayer", window, ecsManager, resourceManager)
        {
            DebugInputManager = {};
            
            //do it after all entities set up
            CurrentECSManager->Refresh();

			Point2 debugRTResolution = Point2(512, 512);
            
            resourceManager->CreateRenderTarget("DebugRT_1", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_2", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_3", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_4", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_5", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_6", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_7", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_8", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            resourceManager->CreateRenderTarget("DebugRT_9", debugRTResolution.x, debugRTResolution.y, TextureFormat::R32G32B32A32_FLOAT);
            
            UpdateSystems.Add((ISystem*)new DebugSystem(CurrentECSManager));
            // UpdateSystems.Add((ISystem*)new LinesRenderingSystem(CurrentECSManager));
            UpdateSystems.Add((ISystem*)new CollisionRenderingSystem(CurrentECSManager));
        	
			SceneData sceneData = { window };
            
            for (ISystem* system : UpdateSystems)
            {
                system->Initialize(&sceneData, &DebugInputManager, CurrentResourceManager);
            }

            DebugInputManager.SubscribeToKeyEvent(WD_KeyCode::F1, [&](bool isPressed)
            {
                if(isPressed)
                {
                    EnableDebug = !EnableDebug;
                }
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
        
        void OnEvent(Event& event) override
        {
            auto eventType = event.GetEventType();

            switch (eventType)
            {
            case EventType::KeyPressed:
            case EventType::KeyReleased:
            case EventType::KeyTyped:
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
            case EventType::MouseMoved:
            case EventType::MouseScrolled:
                {
                    event.Handled = DebugInputManager.Broadcast(event);
                }
            }
        }

        void OnDrawUI(float deltaTime) override
        {
        }

        void OnUpdate(float deltaTime) override
        {
            if(EnableDebug)
            {
                for (ISystem* system : UpdateSystems)
                {
                    system->Update(deltaTime);
                }
            }
        }

    private:
		WArray<ISystem*> UpdateSystems;
        InputManager DebugInputManager;
    };
}