#pragma once
#include "Pipeline.h"
#include "Model/Mesh.h"
#include "Model/Model.h"

namespace Waldem
{
    enum class RendererAPI
    {
        None = 0,
        OpenGL = 1,
        DirectX = 2,
        Vulkan = 3,
    };

    class Renderer
    {
    public:
        static void Initialize();
        static void DrawMesh(Mesh* mesh);
        static void DrawMesh(Pipeline* pipeline, Mesh* mesh);
        static void DrawModel(Pipeline* pipeline, Model* model);
        inline static RendererAPI GetAPI() { return RAPI; }
        static RendererAPI RAPI;
    };
}
