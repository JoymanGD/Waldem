#include "wdpch.h"
#include "Scene.h"

namespace Waldem
{
    void Scene::DrawInternal(SceneData* sceneData)
    {
        if(PreDraw) PreDraw();
        Draw(sceneData);
        if(PostDraw) PostDraw();
    }

    void Scene::UpdateInternal(float deltaTime)
    {
        Update(deltaTime);
    }
}
