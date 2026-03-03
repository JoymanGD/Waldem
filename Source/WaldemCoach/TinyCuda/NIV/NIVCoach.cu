#include "NIVCoach.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "json/json.hpp"
#include <cuda_runtime.h>
#include <tiny-cuda-nn/config.h>
#include <tiny-cuda-nn/gpu_matrix.h>

namespace Waldem
{
    namespace Coach
    {
        namespace TinyCuda
        {
            struct NIVCoach::Impl
            {
                tcnn::TrainableModel Model;
            };

            NIVCoach::NIVCoach() : ImplPtr(std::make_unique<Impl>())
            {
            }

            void NIVCoach::Initialize(uint32_t inputDims, uint32_t outputDims, uint32_t batchSize, uint32_t trainingSteps)
            {
                try
                {
                    std::cout << "[NIV] Initialize: start" << std::endl;
                    InputDims = inputDims;
                    OutputDims = outputDims;
                    BatchSize = batchSize;
                    TrainingSteps = trainingSteps;

                    int deviceCount = 0;
                    cudaError_t cudaErr = cudaGetDeviceCount(&deviceCount);
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaGetDeviceCount failed: ") + cudaGetErrorString(cudaErr));
                    }
                    if (deviceCount <= 0)
                    {
                        throw std::runtime_error("[NIV] No CUDA devices found.");
                    }

                    int activeDevice = 0;
                    cudaErr = cudaGetDevice(&activeDevice);
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaGetDevice failed: ") + cudaGetErrorString(cudaErr));
                    }

                    cudaDeviceProp prop{};
                    cudaErr = cudaGetDeviceProperties(&prop, activeDevice);
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaGetDeviceProperties failed: ") + cudaGetErrorString(cudaErr));
                    }

                    std::cout << "[NIV] CUDA device " << activeDevice << ": " << prop.name << " (cc " << prop.major << "." << prop.minor << ")" << std::endl;

                    std::cout << "[NIV] Working directory: " << std::filesystem::current_path().string() << std::endl;
                    const std::vector<std::filesystem::path> candidatePaths =
                    {
                        std::filesystem::path("Configs") / "default.json",
                        std::filesystem::path("Source") / "WaldemCoach" / "TinyCuda" / "NIV" / "Configs" / "default.json",
                        std::filesystem::path("..") / "Source" / "WaldemCoach" / "TinyCuda" / "NIV" / "Configs" / "default.json",
                        std::filesystem::path("..") / ".." / "Source" / "WaldemCoach" / "TinyCuda" / "NIV" / "Configs" / "default.json",
                        std::filesystem::path("..") / ".." / ".." / "Source" / "WaldemCoach" / "TinyCuda" / "NIV" / "Configs" / "default.json"
                    };

                    std::filesystem::path resolvedConfigPath;
                    for (const auto& candidate : candidatePaths)
                    {
                        const auto absolute = std::filesystem::absolute(candidate);
                        std::cout << "[NIV] Probe config path: " << absolute.string() << std::endl;
                        if (std::filesystem::exists(absolute))
                        {
                            resolvedConfigPath = absolute;
                            break;
                        }
                    }

                    if (resolvedConfigPath.empty())
                    {
                        throw std::runtime_error("[NIV] Failed to find default.json in known locations.");
                    }

                    std::cout << "[NIV] Using config: " << resolvedConfigPath.string() << std::endl;

                    std::ifstream f(resolvedConfigPath);
                    if (!f.is_open())
                    {
                        throw std::runtime_error(std::string("[NIV] Found config but failed to open: ") + resolvedConfigPath.string());
                    }

                    nlohmann::json config;
                    try
                    {
                        f >> config;
                    }
                    catch (const std::exception& e)
                    {
                        throw std::runtime_error(std::string("[NIV] JSON parse failed: ") + e.what());
                    }

                    std::cout << "[NIV] JSON keys:";
                    for (auto it = config.begin(); it != config.end(); ++it)
                    {
                        std::cout << " " << it.key();
                    }
                    std::cout << std::endl;

                    ImplPtr->Model = tcnn::create_from_config(InputDims, OutputDims, config);
                    cudaErr = cudaDeviceSynchronize();
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaDeviceSynchronize after create_from_config failed: ") + cudaGetErrorString(cudaErr));
                    }

                    std::cout << "[NIV] Initialize: finished" << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[NIV] Initialize exception: " << e.what() << std::endl;
                    throw;
                }
                catch (...)
                {
                    std::cerr << "[NIV] Initialize unknown exception." << std::endl;
                    throw;
                }
            }

            void NIVCoach::Train()
            {
                try
                {
                    std::cout << "[NIV] Train: start" << std::endl;
                    std::cout << "[NIV] dims in/out=" << InputDims << "/" << OutputDims << ", batch=" << BatchSize << ", steps=" << TrainingSteps << std::endl;

                    if (BatchSize % tcnn::BATCH_SIZE_GRANULARITY != 0)
                    {
                        throw std::runtime_error("[NIV] BatchSize must be a multiple of BATCH_SIZE_GRANULARITY (" + std::to_string(tcnn::BATCH_SIZE_GRANULARITY) + "). Current BatchSize=" + std::to_string(BatchSize));
                    }

                    tcnn::GPUMatrix<float> training_batch_inputs(InputDims, BatchSize);
                    tcnn::GPUMatrix<float> training_batch_targets(OutputDims, BatchSize);

                    for (uint32_t i = 0; i < TrainingSteps; ++i)
                    {
                        auto ctx = ImplPtr->Model.trainer->training_step(training_batch_inputs, training_batch_targets);
                        float loss = ImplPtr->Model.trainer->loss(*ctx);
                        std::cout << "[NIV] iteration=" << i << " loss=" << loss << std::endl;
                    }

                    const cudaError_t cudaErr = cudaDeviceSynchronize();
                    if (cudaErr != cudaSuccess)
                    {
                        throw std::runtime_error(std::string("[NIV] cudaDeviceSynchronize after training failed: ") + cudaGetErrorString(cudaErr));
                    }

                    std::cout << "[NIV] Train: finished" << std::endl;
                }
                catch (const std::exception& e)
                {
                    std::cerr << "[NIV] Train exception: " << e.what() << std::endl;
                    throw;
                }
                catch (...)
                {
                    std::cerr << "[NIV] Train unknown exception." << std::endl;
                    throw;
                }
            }
        }
    }
}
