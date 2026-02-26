#pragma once

#include "Waldem/Core.h"
#include "Waldem/Window.h"
#include "Waldem/Events/Event.h"
#include "Waldem/ECS/Systems/System.h"

namespace Waldem
{
    class WALDEM_API Layer
    {
    public:
        Layer(const WString& name = "Layer", CWindow* window = nullptr) : DebugName(name), MainWindow(window) {} 
        virtual ~Layer() = default;
        virtual void Begin() {}
        virtual void End() {}
        virtual void Draw() {}
        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnEvent(Event& event) {}
        
        virtual void Initialize()
        {
            for (ISystem* system : UISystems)
            {
                system->Initialize(&InputManager);
            }
        	
            for (ISystem* system : UpdateSystems)
            {
                system->Initialize(&InputManager);
            }
        	
            for (ISystem* system : DrawSystems)
            {
                system->Initialize(&InputManager);
            }
			
            for (ISystem* system : PhysicsSystems)
            {
                system->Initialize(&InputManager);
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
        InputManager InputManager;
        
        WArray<ISystem*> UISystems;
        WArray<ISystem*> UpdateSystems;
        WArray<ISystem*> DrawSystems;
		WArray<ISystem*> PhysicsSystems;
    };
}