#pragma once
#include "../TinyCoach.h"
#include <memory>

namespace Waldem
{
    namespace Coach
    {
        namespace TinyCuda
        {
            class NIVCoach : public TinyCoach
            {
            public:
                NIVCoach();

                void Initialize(uint32_t inputDims, uint32_t outputDims, uint32_t batchSize, uint32_t trainingSteps) override;
                void Train() override;

            private:
                struct Impl;
                std::unique_ptr<Impl> ImplPtr;
                uint32_t InputDims = 1;
                uint32_t OutputDims = 1;
                uint32_t BatchSize = 1;
                uint32_t TrainingSteps = 1;
            };
        }
    }
}
