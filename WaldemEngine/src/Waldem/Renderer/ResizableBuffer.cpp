#include "wdpch.h"
#include "ResizableBuffer.h"
#include "Renderer.h"

namespace Waldem
{
    ResizableBuffer::ResizableBuffer(const WString& name, BufferType type, uint stride, uint capacity) : Stride(stride), Capacity(stride*capacity)
    {
        InternalBuffer = Renderer::CreateBuffer(name, type, Capacity, Stride);
    }

    size_t ResizableBuffer::AddData(void* data, uint size)
    {
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

        if(data)
        {
            Renderer::UploadBuffer(InternalBuffer, data, size, Size);
        }

        size_t offset = Size;
        
        Size = newSize;

        return offset;
    }

    void ResizableBuffer::RemoveData(uint size, uint offset)
    {
        if (offset + size > Size)
        {
            WD_CORE_ERROR("RemoveData: Offset + Size exceeds buffer size.");
        }

        Size -= size;
        
        Renderer::ClearBuffer(InternalBuffer, size, offset);
    }

    void ResizableBuffer::UpdateData(void* data, uint size, uint offset)
    {
        if (offset + size > Size)
        {
            WD_CORE_ERROR("UpdateData: Offset + Size exceeds buffer size.");
        }
        
        Renderer::UploadBuffer(InternalBuffer, data, size, offset);
    }

    void ResizableBuffer::UpdateOrAdd(void* data, uint size, uint offset)
    {
        uint requiredSize = offset + size;

        if (requiredSize > Capacity)
        {
            // Grow capacity like AddData does
            Capacity = std::max(Capacity * 2, requiredSize);

            auto oldBuffer = InternalBuffer;
            InternalBuffer = Renderer::CreateBuffer(InternalBuffer->GetName(),
                                                    InternalBuffer->GetType(),
                                                    Capacity,
                                                    Stride);

            if(Size > 0)
            {
                Renderer::CopyBufferRegion(InternalBuffer, 0, oldBuffer, 0, Size);
            }

            Renderer::Destroy(oldBuffer);
            delete oldBuffer;
        }

        if (data)
        {
            Renderer::UploadBuffer(InternalBuffer, data, size, offset);
        }

        // Extend logical size if we wrote beyond current Size
        if (requiredSize > Size)
        {
            Size = requiredSize;
        }
    }
}
