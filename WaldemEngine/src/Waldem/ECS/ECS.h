#pragma once

#include "flecs.h"

namespace Waldem
{
    namespace ECS
    {
        inline flecs::world World;
        
        void RegisterComponents();

        class Core
        {
        public:
            void Initialize()
            {
                World = flecs::world();
                RegisterComponents();
            }
        };
    }
}
