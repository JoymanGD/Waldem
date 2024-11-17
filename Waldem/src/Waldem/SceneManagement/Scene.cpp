#include "wdpch.h"
#include "Scene.h"

namespace Waldem
{
    void Scene::DrawInternal(SceneData* sceneData, float deltaTime)
    {
        if(PreDraw) PreDraw();
        Draw(sceneData, deltaTime);
        if(PostDraw) PostDraw();
    }

    void Scene::UpdateInternal(SceneData* sceneData, float deltaTime)
    {
        Update(sceneData, deltaTime);
    }
}
