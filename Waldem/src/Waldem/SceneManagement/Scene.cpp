#include "wdpch.h"
#include "Scene.h"

#include "glad/glad.h"

namespace Waldem
{
    void Scene::DrawInternal()
    {
        Clear();
        
        if(PreDraw) PreDraw();
        Draw();
        if(PostDraw) PostDraw();
    }

    void Scene::UpdateInternal(float deltaTime)
    {
        Update(deltaTime);
    }

    void Scene::SetClearColor(glm::vec4 Color)
    {
        ClearColor = Color;
    }

    void Scene::Clear()
    {
        glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}