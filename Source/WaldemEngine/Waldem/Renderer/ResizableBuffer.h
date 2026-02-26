#pragma once
#include "Buffer.h"

namespace Waldem
{
    class WALDEM_API ResizableBuffer
    {
    public:
        uint Stride = 0;
        uint Capacity = 0;
        uint Size = 0;

        ResizableBuffer() = default;

        ResizableBuffer(const WString& name, BufferType type, uint stride, uint capacity = 51);

        Buffer* GetBuffer() { return InternalBuffer; }

        operator Buffer*() const { return InternalBuffer; }
        operator Buffer*() { return InternalBuffer; }

        size_t AddData(void* data, uint size);

        void RemoveData(uint size, uint offset);

        void UpdateData(void* data, uint size, uint offset = 0);

        void UpdateOrAdd(void* data, uint size, uint offset);

        uint64 GetGPUAddress() const { return InternalBuffer->GetGPUAddress(); }
        uint GetIndex(ResourceHeapType heapType) { return InternalBuffer->GetIndex(heapType); }
        
    private:
        Buffer* InternalBuffer = nullptr;
    };
}
