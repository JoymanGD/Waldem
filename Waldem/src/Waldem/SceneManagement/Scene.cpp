#include "wdpch.h"
#include "Scene.h"

namespace Waldem
{
    void Scene::DrawInternal(Renderer* renderer)
    {
        if(PreDraw) PreDraw();
        Draw(renderer);
        if(PostDraw) PostDraw();
    }

    void Scene::UpdateInternal(float deltaTime)
    {
        Update(deltaTime);
    }
}
