#pragma once
#include <cstdint>

namespace Waldem
{
    namespace Coach
    {
        class ICoach
        {
        public:
            virtual void Initialize(uint32_t inputDims, uint32_t outputDims, uint32_t batchSize, uint32_t trainingSteps) = 0;
            virtual void Train() = 0;
        };
    }
}
