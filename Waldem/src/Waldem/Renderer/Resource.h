#pragma once
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
        uint32_t NumResources = 1;
        void* Data = nullptr;
        uint32_t Stride = 0;
        Vector2 Size;
        uint32_t Slot = 0;
        WArray<Texture2D*> Textures;
        WArray<StorageBuffer*> Buffers;
        WArray<Sampler> Samplers;
        RenderTarget* RT = nullptr;

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

        //constants
        Resource(String name, ResourceType type, uint32_t numConstants, void* data, uint32_t slot)
        {
            Name = name;
            Type = type;
            NumResources = 1;
            Data = data;
            uint32_t stride = sizeof(uint32_t);
            Stride = stride;
            Size = Vector2(stride * numConstants, 1);
            Slot = slot;
        }

        //textures
        Resource(String name, WArray<Texture2D*> textures, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Texture;
            Textures = textures;
            NumResources = textures.Num();
            Slot = slot;
        }

        //buffers
        Resource(String name, WArray<StorageBuffer*> buffers, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Buffer;
            Buffers = buffers;
            NumResources = buffers.Num();
            Slot = slot;
        }

        //render targets
        Resource(String name, RenderTarget* renderTarget, uint32_t slot)
        {
            Name = name;
            // Type = renderTarget->IsDepthStencilBuffer() ? RTYPE_RenderTarget : RTYPE_RWRenderTarget;
            Type = RTYPE_RenderTarget;
            RT = renderTarget;
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
