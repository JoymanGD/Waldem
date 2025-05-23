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
        Buffer(WString name, BufferType type, size_t size, uint32_t stride) : Name(name), Type(type), Size(size), Stride(stride), Count(size/stride) { SetType(RTYPE_Buffer); }
        virtual ~Buffer() {}

        virtual uint32_t GetCount() const { return Count; }
        size_t GetSize() { return Size; }
        uint32_t GetStride() { return Stride; }
        BufferType GetType() { return Type; }
        WString GetName() { return Name; }

    protected:
        WString Name;
        BufferType Type;
        size_t Size;
        uint32_t Stride;
        uint32_t Count;
    };
}
