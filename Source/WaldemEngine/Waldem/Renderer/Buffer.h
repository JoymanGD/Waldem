#pragma once
#include "GraphicResource.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    enum BufferType
    {
        VertexBuffer = 0,
        IndexBuffer = 1,
        StorageBuffer = 2,
        IndirectBuffer = 3
    };

    class Buffer : public GraphicResource
    {
    public:
        Buffer() { SetType(RTYPE_Buffer); }
        Buffer(WString name, BufferType type, size_t capacity, uint32_t stride) : Name(name), Type(type), Capacity(capacity), Stride(stride) { SetType(RTYPE_Buffer); }
        virtual ~Buffer() {}

        void AddSize(size_t size) { Size += size; }
        size_t GetCapacity() { return Capacity; }
        uint32_t GetCount() { return Capacity / Stride; }
        uint32_t GetStride() { return Stride; }
        BufferType GetType() { return Type; }
        WString GetName() { return Name; }

    protected:
        WString Name;
        BufferType Type;
        size_t Capacity = 0;
        size_t Size = 0;
        uint32_t Stride = 0;
    };
}
