#pragma once
#include "AccelerationStructure.h"
#include "Buffer.h"
#include "Sampler.h"
#include "Shader.h"
#include "Texture.h"

namespace Waldem
{
    class WALDEM_API Resource
    {
    public:
        String Name;
        ResourceType Type;
        bool IsArray = false;
        uint32_t NumResources = 1;
        void* Data = nullptr;
        uint32_t Stride = 0;
        Vector2 Size;
        uint32_t Slot = 0;
        WArray<Texture2D*> Textures;
        WArray<Buffer*> Buffers;
        WArray<Sampler> Samplers;
        RenderTarget* RT = nullptr;
        AccelerationStructure* AS = nullptr;

        Resource() = default;
        
        //constant buffers
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

        //constant buffers
        //TODO: remove size, make it always 1
        Resource(String name, ResourceType type, void* data, uint32_t stride, float size, uint32_t slot)
        {
            Name = name;
            Type = type;
            NumResources = 1;
            Data = data;
            Stride = stride;
            if(size == 0) size = 1;
            Size = Vector2(size, 1);
            Slot = slot;
        }

        //textures
        Resource(String name, WArray<Texture2D*> textures, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Texture;
            IsArray = true;
            Textures = textures;
            NumResources = textures.Num();
            Slot = slot;
        }
        
        Resource(String name, Texture2D* texture, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Texture;
            IsArray = false;
            Textures.Add(texture);
            NumResources = 1;
            Slot = slot;
        }

        //buffers
        Resource(String name, WArray<Buffer*> buffers, uint32_t slot, bool UAV = false)
        {
            Name = name;
            Type = UAV ? RTYPE_RWBuffer : RTYPE_Buffer;
            Buffers = buffers;
            Size = Vector2(buffers[0]->GetSize(), 1);
            Stride = Size.x / buffers[0]->GetCount();
            NumResources = buffers.Num();
            Slot = slot;
        }
        
        Resource(String name, Buffer* buffer, uint32_t slot, bool UAV = false)
        {
            Name = name;
            Type = UAV ? RTYPE_RWBuffer : RTYPE_Buffer;
            Buffers.Add(buffer);
            Size = Vector2(buffer->GetSize(), 1);
            Stride = Size.x / buffer->GetCount();
            NumResources = 1;
            Slot = slot;
        }

        //render targets
        Resource(String name, RenderTarget* renderTarget, uint32_t slot, bool UAV = false)
        {
            Name = name;
            Type = UAV ? RTYPE_RWRenderTarget : RTYPE_RenderTarget;
            RT = renderTarget;
            NumResources = 1;
            Slot = slot;
        }

        //acceleration structure
        Resource(String name, AccelerationStructure* accelerationStructure, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_AccelerationStructure;
            AS = accelerationStructure;
            NumResources = 1;
            Slot = slot;
        }

        //samplers
        Resource(String name, WArray<Sampler> samplers, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Sampler;
            Samplers = samplers;
            NumResources = samplers.Num();
            Slot = slot;
        }
    };
}
