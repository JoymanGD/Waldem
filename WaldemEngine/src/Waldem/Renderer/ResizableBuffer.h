#pragma once
#include "Buffer.h"
#include "Renderer.h"

namespace Waldem
{
    class WALDEM_API ResizableBuffer
    {
    public:
        uint Stride = 0;
        uint Capacity = 0;
        uint Size = 0;

        ResizableBuffer() = default;

        ResizableBuffer(const WString& name, BufferType type, uint stride, uint capacity = 51) : Stride(stride), Capacity(stride*capacity)
        {
            InternalBuffer = Renderer::CreateBuffer(name, type, Capacity, Stride);
        }

        Buffer* GetBuffer() { return InternalBuffer; }

        operator Buffer*() const { return InternalBuffer; }
        operator Buffer*() { return InternalBuffer; }

        size_t AddData(void* data, uint size)
        {
            Count += size / Stride;
            
            uint newSize = Size + size;
            
            if (newSize > Capacity)
            {
                // Resize the buffer
                Capacity += newSize * 2;
                
                // void* previousData = nullptr;
                //
                // if(Size > 0)
                // {
                //     previousData = malloc(Size);
                //     Renderer::DownloadBuffer(InternalBuffer, previousData, Size);
                // }
                //
                // Renderer::InitializeBuffer(InternalBuffer->GetName(), InternalBuffer->GetType(), Capacity, Stride, InternalBuffer, previousData, Size);

                auto oldBuffer = InternalBuffer;
                InternalBuffer = Renderer::CreateBuffer(InternalBuffer->GetName(), InternalBuffer->GetType(), Capacity, Stride);

                if(Size > 0)
                {
                    Renderer::CopyBufferRegion(InternalBuffer, 0, oldBuffer, 0, Size);
                }

                Renderer::Destroy(oldBuffer);
                delete oldBuffer;
            }
            
            Renderer::UploadBuffer(InternalBuffer, data, size, Size);

            size_t offset = Size;
            
            Size = newSize;

            return offset;
        }

        void UpdateData(void* data, uint size, uint offset = 0)
        {
            if (offset + size > Size)
            {
                WD_CORE_ERROR("UpdateData: Offset + Size exceeds buffer size.");
            }
            
            Renderer::UploadBuffer(InternalBuffer, data, size, offset);
        }

        uint64 GetGPUAddress() const { return InternalBuffer->GetGPUAddress(); }
        uint GetIndex(ResourceHeapType heapType) { return InternalBuffer->GetIndex(heapType); }
        uint Num() { return Count; }
        
    private:
        Buffer* InternalBuffer = nullptr;
        int Count = 0;
    };
}
