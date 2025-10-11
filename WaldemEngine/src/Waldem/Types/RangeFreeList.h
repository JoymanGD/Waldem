#pragma once
#include <set>
#include <utility>
#include <algorithm>

namespace Waldem
{
    class RangeFreeList
    {
    public:
        RangeFreeList() = default;

        void Initialize(uint capacity)
        {
            FreeRanges.clear();
            FreeRanges.insert({0, capacity});
            TotalCapacity = capacity;
        }

        uint Allocate(uint count)
        {
            for (auto it = FreeRanges.begin(); it != FreeRanges.end(); ++it)
            {
                uint start = it->first;
                uint length = it->second;

                if (length >= count)
                {
                    uint offset = start;
                    uint remaining = length - count;

                    FreeRanges.erase(it);
                    if (remaining > 0)
                        FreeRanges.insert({start + count, remaining});
                    return offset;
                }
            }

            Expand(std::max(TotalCapacity * 2, TotalCapacity + count));
            return Allocate(count);
        }

        void Free(uint offset, uint count)
        {
            FreeRanges.insert({offset, count});
            MergeAdjacent();
        }

        void Expand(uint newCapacity)
        {
            if (newCapacity <= TotalCapacity)
                return;

            FreeRanges.insert({TotalCapacity, newCapacity - TotalCapacity});
            TotalCapacity = newCapacity;
            MergeAdjacent();
        }

        uint GetTotalCapacity() const
        {
            return TotalCapacity;
        }

    private:
        std::set<std::pair<uint, uint>> FreeRanges;
        uint TotalCapacity = 0;

        void MergeAdjacent()
        {
            if (FreeRanges.empty())
                return;

            auto it = FreeRanges.begin();
            auto next = std::next(it);

            while (next != FreeRanges.end())
            {
                if (it->first + it->second == next->first)
                {
                    uint newStart = it->first;
                    uint newLen = it->second + next->second;

                    it = FreeRanges.erase(it);
                    next = FreeRanges.erase(next);
                    it = FreeRanges.insert({newStart, newLen}).first;
                    next = std::next(it);
                }
                else
                {
                    ++it;
                    ++next;
                }
            }
        }
    };
}
