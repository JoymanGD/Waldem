#pragma once
#include <set>

namespace Waldem
{
    class WALDEM_API FreeList
    {
    public:
        FreeList() : NextIndex(0) {}

        int Allocate()
        {
            if (!FreeIndices.empty())
            {
                auto it = FreeIndices.begin(); // smallest
                int idx = *it;
                FreeIndices.erase(it);
                return idx;
            }
            return NextIndex++;
        }

        bool IsAllocated(int index) const
        {
            return index < NextIndex && FreeIndices.find(index) == FreeIndices.end();
        }

        void MarkAsAllocated(int index)
        {
            if (index >= NextIndex)
            {
                NextIndex = index + 1;
            }
            FreeIndices.erase(index);
        }

        void Free(int index)
        {
            FreeIndices.insert(index);
        }

        void Clear()
        {
            NextIndex = 0;
            FreeIndices.clear();
        }

    private:
        int NextIndex;
        std::set<int> FreeIndices;
    };
}
