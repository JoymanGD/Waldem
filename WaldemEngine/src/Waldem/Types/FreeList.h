#pragma once
#include <queue>

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
                int idx = FreeIndices.front();
                FreeIndices.pop();
                return idx;
            }
            return NextIndex++;
        }

        void Free(int index)
        {
            FreeIndices.push(index);
        }

        void Clear()
        {
            NextIndex = 0;
            std::queue<int> empty;
            std::swap(FreeIndices, empty);
        }

    private:
        int NextIndex;
        std::queue<int> FreeIndices;
    };
}
