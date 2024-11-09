#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"

namespace Waldem
{
    class WALDEM_API Resource
    {
    public:
        String Name;
        ResourceType Type;
        uint32_t NumResources = 1;
        void* Data = nullptr;
        uint32_t Stride = 0;
        Vector2 Size;
        uint32_t Slot = 0;
        std::vector<Texture2D*> Textures;
        std::vector<StorageBuffer*> Buffers;
        RenderTarget* RT = nullptr;

        Resource(String name, ResourceType type, void* data, uint32_t stride, Vector2 size, uint32_t slot)
        {
            Name = name;
            Type = type;
            NumResources = 1;
            Data = data;
            Stride = stride;
            Size = size;
            Slot = slot;
        }
        
        Resource(String name, ResourceType type, void* data, uint32_t stride, float size, uint32_t slot)
        {
            Name = name;
            Type = type;
            NumResources = 1;
            Data = data;
            Stride = stride;
            Size = Vector2(size, 1);
            Slot = slot;
        }
        
        Resource(String name, std::vector<Texture2D*> textures, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Texture;
            Textures = textures;
            NumResources = textures.size();
            Slot = slot;
        }
        
        Resource(String name, std::vector<StorageBuffer*> buffers, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Buffer;
            Buffers = buffers;
            NumResources = buffers.size();
            Slot = slot;
        }
        
        Resource(String name, RenderTarget* renderTarget, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_RenderTarget;
            RT = renderTarget;
            NumResources = 1;
            Slot = slot;
        }
    };
}
