#pragma once
#include <queue>

namespace Waldem
{
    class WALDEM_API DescriptorAllocator
    {
    public:
        DescriptorAllocator(uint32_t capacity) : Capacity(capacity)
        {
            for (uint32_t i = 0; i < Capacity; ++i)
            {
                FreeIndices.push(i);
            }
        }

        uint32_t Allocate()
        {
            std::lock_guard<std::mutex> lock(AllocatorMutex);

            assert(!FreeIndices.empty() && "DescriptorAllocator overflow!");
            uint32_t index = FreeIndices.front();
            FreeIndices.pop();
            return index;
        }

        void Free(uint32_t index)
        {
            std::lock_guard<std::mutex> lock(AllocatorMutex);

            assert(index < Capacity && "Invalid descriptor index freed!");
            FreeIndices.push(index);
        }

        uint32_t GetCapacity() const { return Capacity; }

        uint32_t GetFreeCount() const
        {
            std::lock_guard<std::mutex> lock(AllocatorMutex);
            return static_cast<uint32_t>(FreeIndices.size());
        }

    private:
        uint32_t Capacity;
        std::queue<uint32_t> FreeIndices;
        mutable std::mutex AllocatorMutex; // Optional thread safety
    };
}