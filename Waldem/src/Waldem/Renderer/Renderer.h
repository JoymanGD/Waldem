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

    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;
        virtual void Initialize() = 0;
        virtual void Clear(glm::vec4 clearColor) = 0;
        virtual void Render(uint32_t indexCount) = 0;
    };

    class Renderer
    {
    public:
        void Initialize();
        void Clear();
        void DrawMesh(Mesh* mesh);
        void DrawMesh(Pipeline* pipeline, Mesh* mesh);
        void DrawModel(Pipeline* pipeline, Model* model);
        
        void CreateStorageBuffer(void* data, size_t size)
        {
            StorageBuffer::Create(data, size);
        }
        
        static RendererAPI RAPI;
    private:
        IRenderer* CurrentRenderer;
        glm::vec4 ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    };
}
