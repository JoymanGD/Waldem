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
        DebugLayer(CWindow* window, flecs::world* ecsManager, ResourceManager* resourceManager) : Layer("DebugLayer", window, resourceManager)
        {
            InputManager = {};
            
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
            
            UpdateSystems.Add(new DebugSystem());
            // UpdateSystems.Add((ISystem*)new LinesRenderingSystem(CurrentECSManager));
            UpdateSystems.Add(new CollisionRenderingSystem());

            InputManager.SubscribeToKeyEvent(WD_KeyCode::F1, [&](bool isPressed)
            {
                if(isPressed)
                {
                    EnableDebug = !EnableDebug;
                }
            });
        }

        void Initialize() override
        {
            for (ISystem* system : UpdateSystems)
            {
                system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
            }

			Initialized = true;
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
                    event.Handled = InputManager.Broadcast(event);
                }
            }
        }
    };
}