#pragma once
#include "Scene.h"

namespace Waldem
{
    class WALDEM_API SceneManager
    {
    public:
        IScene* GetCurrentScene();
    private:
    };
}
