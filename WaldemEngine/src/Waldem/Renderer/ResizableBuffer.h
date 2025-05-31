#pragma once
#include "Buffer.h"
#include "Renderer.h"

namespace Waldem
{
    class WALDEM_API ResizableBuffer
    {
    public:
        uint Stride = 0;
        uint Size = 0;
        uint ThresholdNum = 0;

        ResizableBuffer() = default;
        
        ResizableBuffer(const WString& name, BufferType type, uint stride, uint thresholdNum, uint size = 0, void* data = nullptr) : Stride(stride), Size(size), ThresholdNum(thresholdNum)
        {
            InternalBuffer = Renderer::CreateBuffer(name, type, data, stride*thresholdNum, stride);
        }

        void AddData(void* data, uint size)
        {
            uint newSize = Size + size;
            
            if (newSize > ThresholdNum * Stride)
            {
                // Resize the buffer
                Buffer* newBuffer = Renderer::CreateBuffer(InternalBuffer->GetName(), InternalBuffer->GetType(), nullptr, newSize, Stride);
                Renderer::CopyResource(newBuffer, InternalBuffer);
                Renderer::Destroy(InternalBuffer);
                delete InternalBuffer;
                InternalBuffer = newBuffer;

                ThresholdNum += ThresholdNum;
            }
            
            Renderer::UploadBuffer(InternalBuffer, data, size, Size);
            
            Size = newSize;
        }

        operator Buffer*() const { return InternalBuffer; }
        
        uint64 GetGPUAddress() const { return InternalBuffer->GetGPUAddress(); }
        uint GetIndex(ResourceHeapType heapType) { return InternalBuffer->GetIndex(heapType); }
        
    private:
        Buffer* InternalBuffer = nullptr;
    };
}
