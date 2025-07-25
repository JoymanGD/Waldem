#pragma once

#include "Waldem/Core.h"
#include "Waldem/Window.h"
#include "Waldem/Events/Event.h"
#include "Waldem/ECS/ECSManager.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
#include "Waldem/Resources/ResourceManager.h"
#include "Waldem/SceneManagement/Scene.h"

namespace Waldem
{
    class WALDEM_API Layer
    {
    public:
        Layer(const WString& name = "Layer", CWindow* window = nullptr, ECSManager* ecsManager = nullptr, ResourceManager* resourceManager = nullptr) : DebugName(name), MainWindow(window), CurrentECSManager(ecsManager), CurrentResourceManager(resourceManager) {}
        virtual ~Layer() = default;
        virtual void Begin() {}
        virtual void End() {}
        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(float deltaTime) {}
        virtual void OnDraw(float deltaTime) {}
        virtual void OnFixedUpdate(float deltaTime) {}
        virtual void OnDrawUI(float deltaTime) {}
        virtual void OnEvent(Event& event) {}
        
        virtual void Initialize()
        {
            for (ISystem* system : UISystems)
            {
                system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
            }
        	
            for (ISystem* system : UpdateSystems)
            {
                system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
            }
        	
            for (ISystem* system : DrawSystems)
            {
                system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
            }
			
            for (ISystem* system : PhysicsSystems)
            {
                system->Initialize(&InputManager, CurrentResourceManager, &ContentManager);
            }

            Initialized = true;
        }
        
        virtual void Deinitialize()
        {
            for (ISystem* system : UISystems)
            {
                system->Deinitialize();
            }
        	
            for (ISystem* system : UpdateSystems)
            {
                system->Deinitialize();
            }
        	
            for (ISystem* system : DrawSystems)
            {
                system->Deinitialize();
            }
			
            for (ISystem* system : PhysicsSystems)
            {
                system->Deinitialize();
            }

            Initialized = false;
        }

        inline const WString& GetName() const { return DebugName; }

        bool Initialized = false;
        
    protected:
        WString DebugName;
        CWindow* MainWindow;
        ECSManager* CurrentECSManager;
        ResourceManager* CurrentResourceManager;
        InputManager InputManager;
        CContentManager ContentManager;
        
        WArray<ISystem*> UISystems;
        WArray<ISystem*> UpdateSystems;
        WArray<DrawSystem*> DrawSystems;
		WArray<ISystem*> PhysicsSystems;
    };
}