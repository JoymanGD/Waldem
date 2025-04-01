#include <wdpch.h>
#include <SDL.h>
#include "Waldem/PlatformInitializer.h"

namespace Waldem
{
    void PlatformInitializer::Initialize()
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        {
            WD_CORE_ERROR("SDL could not initialize! SDL_Error: {0}", SDL_GetError());
        }
    }
}
