#pragma once
#include "Buffer.h"
#include "Shader.h"
#include "Texture.h"

namespace Waldem
{
    class WALDEM_API Resource
    {
    public:
        std::string Name;
        ResourceType Type;
        uint32_t NumResources = 1;
        void* Data = nullptr;
        uint32_t Stride = 0;
        Vector2 Size;
        uint32_t Slot = 0;
        std::vector<Texture2D*> Textures;
        std::vector<StorageBuffer*> Buffers;

        Resource(std::string name, ResourceType type, uint32_t numResources, void* data, uint32_t stride, Vector2 size, uint32_t slot)
        {
            Name = name;
            Type = type;
            NumResources = numResources;
            Data = data;
            Stride = stride;
            Size = size;
            Slot = slot;
        }
        
        Resource(std::string name, ResourceType type, uint32_t numResources, void* data, uint32_t stride, float size, uint32_t slot)
        {
            Name = name;
            Type = type;
            NumResources = numResources;
            Data = data;
            Stride = stride;
            Size = Vector2(size, 1);
            Slot = slot;
        }
        
        Resource(std::string name, std::vector<Texture2D*> textures, uint32_t slot)
        {
            Name = name;
            Type = ResourceType::Texture;
            Textures = textures;
            NumResources = textures.size();
            Slot = slot;
        }
        
        Resource(std::string name, std::vector<StorageBuffer*> buffers, uint32_t slot)
        {
            Name = name;
            Type = ResourceType::Buffer;
            Buffers = buffers;
            NumResources = buffers.size();
            Slot = slot;
        }
    };
}
