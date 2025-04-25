#pragma once
#include "Shader.h"
#include "Buffer.h"
#include "Sampler.h"
#include "Texture.h"

namespace Waldem
{
    enum GraphicResourceType
    {
        RTYPE_ConstantBuffer = 0,
        RTYPE_Buffer = 1,
        RTYPE_BufferRaw = 2,
        RTYPE_RWBuffer = 3,
        RTYPE_RWBufferRaw = 4,
        RTYPE_Texture = 5,
        RTYPE_RWTexture = 6,
        RTYPE_Sampler = 7,
        RTYPE_RenderTarget = 8,
        RTYPE_RWRenderTarget = 9,
        RTYPE_AccelerationStructure = 10,
        RTYPE_Constant = 19
    };
    
    class RenderTarget;
    class AccelerationStructure;

    class WALDEM_API GraphicResource
    {
    public:
        WString Name;
        GraphicResourceType Type;
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

        GraphicResource() = default;
        
        //constant buffers
        GraphicResource(WString name, GraphicResourceType type, void* data, uint32_t stride, Vector2 size, uint32_t slot)
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
        GraphicResource(WString name, GraphicResourceType type, void* data, uint32_t stride, float size, uint32_t slot)
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
        GraphicResource(WString name, WArray<Texture2D*> textures, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Texture;
            IsArray = true;
            Textures = textures;
            NumResources = textures.Num();
            Slot = slot;
        }
        
        GraphicResource(WString name, Texture2D* texture, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Texture;
            IsArray = false;
            Textures.Add(texture);
            NumResources = 1;
            Slot = slot;
        }

        //buffers
        GraphicResource(WString name, WArray<Buffer*> buffers, uint32_t slot, bool UAV = false)
        {
            Name = name;
            Type = UAV ? RTYPE_RWBuffer : RTYPE_Buffer;
            Buffers = buffers;
            Size = Vector2(buffers[0]->GetSize(), 1);
            Stride = Size.x / buffers[0]->GetCount();
            NumResources = buffers.Num();
            Slot = slot;
        }
        
        GraphicResource(WString name, Buffer* buffer, uint32_t slot, bool UAV = false)
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
        GraphicResource(WString name, RenderTarget* renderTarget, uint32_t slot, bool UAV = false)
        {
            Name = name;
            Type = UAV ? RTYPE_RWRenderTarget : RTYPE_RenderTarget;
            RT = renderTarget;
            NumResources = 1;
            Slot = slot;
        }

        //acceleration structure
        GraphicResource(WString name, AccelerationStructure* accelerationStructure, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_AccelerationStructure;
            AS = accelerationStructure;
            NumResources = 1;
            Slot = slot;
        }

        //samplers
        GraphicResource(WString name, WArray<Sampler> samplers, uint32_t slot)
        {
            Name = name;
            Type = RTYPE_Sampler;
            Samplers = samplers;
            NumResources = samplers.Num();
            Slot = slot;
        }
    };
}
