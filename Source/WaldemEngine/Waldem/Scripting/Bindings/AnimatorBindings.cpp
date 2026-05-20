#include "wdpch.h"
#include "TimeBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/Time.h"
#include "Waldem/ECS/Components/AnimatorComponent.h"

namespace Waldem::Bindings
{
    namespace
    {
        void Animator_Play(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<AnimatorComponent>()) return;
            auto& animator = entity.get_mut<AnimatorComponent>();
            animator.Play();
            entity.modified<AnimatorComponent>();
        }

        void Animator_Stop(uint64_t entityId)
        {
            ECS::Entity entity = GetEntityChecked(entityId);
            if(!entity.is_alive() || !entity.has<AnimatorComponent>()) return;
            auto& animator = entity.get_mut<AnimatorComponent>();
            animator.Stop();
            entity.modified<AnimatorComponent>();
            
        }
    }

    void RegisterAnimatorCalls(Mono* runtime)
    {
        BIND(runtime, Animator_Play);
        BIND(runtime, Animator_Stop);
    }
}
