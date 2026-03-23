#pragma once
#include "Waldem/Core.h"
#include <cstdint>
#include <string>

namespace Waldem
{
    namespace Coach
    {
        class WALDEM_API ICoach
        {
        public:
            virtual void Initialize(uint32_t inputDims, uint32_t outputDims, uint32_t batchSize, uint32_t trainingSteps, const std::string& datasetPath) = 0;
            virtual void Train() = 0;
        };
    }
}
