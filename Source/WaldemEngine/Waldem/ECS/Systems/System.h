#pragma once

#include "Waldem/ECS/ECSTypes.h"
#include "Waldem/ECS/ECSWorld.h"
#include "Waldem/Input/InputManager.h"

namespace Waldem
{
    class WALDEM_API ISystem
    {
    protected:
        bool IsInitialized = false;
        bool IsActive = true;
        bool AlwaysActive = false;
    public:
        ISystem() {}
        virtual void Initialize() {}
        virtual void Deinitialize() {}
        virtual void OnResize(Vector2 size) {}
        virtual void SetActive(bool value)
        {
            IsActive = value;
        }

        inline bool IsAlwaysActive() { return AlwaysActive; }

        template<typename... Ts>
        struct Components {};

        template<typename... Ts>
        struct Without {};

        template<typename T>
        struct IsSystemPack : std::false_type {};

        template<typename... Ts>
        struct IsSystemPack<Components<Ts...>> : std::true_type {};

        template<typename... Ts>
        struct IsSystemPack<Without<Ts...>> : std::true_type {};

        template<typename Kind, typename... Cs, typename Func>
        requires (!(IsSystemPack<Cs>::value || ...))
        void System(WString name, Func&& func)
        {
            if constexpr (sizeof...(Cs) == 0)
            {
                ECS::World.system(name).template kind<Kind>().run([func = std::forward<Func>(func), this] (flecs::iter& it)
                {
                    if (!IsActive)
                        return;

                    func();
                });
            }
            else
            {
                ECS::World.system<Cs...>(name).template kind<Kind>().each([func = std::forward<Func>(func), this] (ECS::Entity entity, Cs&... components) mutable
                {
                    if (!IsActive)
                        return;

                    if constexpr (std::is_invocable_v<decltype(func)&, ECS::Entity, Cs&...>)
                    {
                        func(entity, components...);
                    }
                    else
                    {
                        func(components...);
                    }
                });
            }
        }

        template<typename Kind, typename ComponentsPack, typename WithoutPack, typename Func>
        requires (IsSystemPack<ComponentsPack>::value && IsSystemPack<WithoutPack>::value)
        void System(WString name, Func&& func)
        {
            [&]<typename... Cs, typename... Excluded>(Components<Cs...>, Without<Excluded...>)
            {
                auto system = ECS::World.system<Cs...>(name).template kind<Kind>();

                (system.template without<Excluded>(), ...);

                system.each([func = std::forward<Func>(func), this] (ECS::Entity entity, Cs&... components) mutable
                {
                    if (!IsActive)
                        return;

                    func(entity, components...);
                });
            } (ComponentsPack{}, WithoutPack{});
        }

        template<typename... Cs, typename Func>
        requires (!(IsSystemPack<Cs>::value || ...))
        void Observer(WString name, flecs::entity_t event, Func&& func)
        {
            ECS::World.observer<Cs...>(name).event(event).each([func = std::forward<Func>(func), this] (ECS::Entity entity, Cs&... components)
            {
                if (!IsActive)
                    return;

                if constexpr (std::is_invocable_v<decltype(func), ECS::Entity, Cs&...>)
                {
                    func(entity, components...);
                }
                else
                {
                    func(components...);
                }
            });
        }

        template<typename ComponentsPack, typename WithoutPack, typename Func>
        requires (IsSystemPack<ComponentsPack>::value && IsSystemPack<WithoutPack>::value)
        void Observer(WString name, flecs::entity_t event, Func&& func)
        {
            [&]<typename... Cs, typename... Excluded>(Components<Cs...>, Without<Excluded...>)
            {
                auto observer = ECS::World.observer<Cs...>(name).event(event);

                (observer.template without<Excluded>(), ...);

                observer.each([func = std::forward<Func>(func), this] (ECS::Entity entity, Cs&... components)
                {
                    if (!IsActive)
                        return;

                    if constexpr (std::is_invocable_v<decltype(func), ECS::Entity, Cs&...>)
                    {
                        func(entity, components...);
                    }
                    else
                    {
                        func(components...);
                    }
                });
            } (ComponentsPack{}, WithoutPack{});
        }
    };
}
