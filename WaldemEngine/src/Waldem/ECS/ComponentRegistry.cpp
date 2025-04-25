#include "wdpch.h"
#include "ComponentRegistry.h"
#include "Components/AudioListener.h"
#include "Components/AudioSource.h"
#include "Components/BloomPostProcess.h"
#include "Components/Camera.h"
#include "Components/ColliderComponent.h"
#include "Components/EditorCamera.h"
#include "Components/Gizmo.h"
#include "Components/Light.h"
#include "Components/LineComponent.h"
#include "Components/MeshComponent.h"
#include "Components/ModelComponent.h"
#include "Components/NameComponent.h"
#include "Components/Ocean.h"
#include "Components/RigidBody.h"
#include "Components/ScriptComponent.h"
#include "Components/Selected.h"
#include "Components/Transform.h"

namespace Waldem
{
    void ComponentRegistry::RegisterAllComponents()
    {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;
        
        REGISTER_COMPONENT(AudioListener);
        REGISTER_COMPONENT(AudioSource);
        REGISTER_COMPONENT(BloomPostProcess);
        REGISTER_COMPONENT(Camera);
        REGISTER_COMPONENT(ColliderComponent);
        REGISTER_COMPONENT(EditorCamera);
        REGISTER_COMPONENT(Gizmo);
        REGISTER_COMPONENT(Light);
        REGISTER_COMPONENT(LineComponent);
        REGISTER_COMPONENT(MeshComponent);
        REGISTER_COMPONENT(ModelComponent);
        REGISTER_COMPONENT(NameComponent);
        REGISTER_COMPONENT(Ocean);
        REGISTER_COMPONENT(RigidBody);
        REGISTER_COMPONENT(ScriptComponent);
        REGISTER_COMPONENT(Selected);
        REGISTER_COMPONENT(Transform);
    }
}
