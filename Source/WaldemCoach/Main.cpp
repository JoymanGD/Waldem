#include <iostream>
#include <string>

#include "Coach.h"
#include "TinyCuda/NIV/NIVCoach.h"

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        std::cerr << "Usage: " << argv[0] << " <coach_name> <input_dims> <output_dims> <batch_size> <training_steps>" << std::endl;
        return 1;
    }

    std::string coachName = argv[1];
    uint32_t inputDims = std::stoul(argv[2]);
    uint32_t outputDims = std::stoul(argv[3]);
    uint32_t batchSize = std::stoul(argv[4]);
    uint32_t trainingSteps = std::stoul(argv[5]);

    Waldem::Coach::ICoach* coach = nullptr;

    if(coachName == "niv")
    {
        coach = (Waldem::Coach::ICoach*)new Waldem::Coach::TinyCuda::NIVCoach();
    }

    if(coach)
    {
        coach->Initialize(inputDims, outputDims, batchSize, trainingSteps);
        coach->Train();
    }
}
