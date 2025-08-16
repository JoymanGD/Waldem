// #pragma once
// #include "Waldem/ECS/Systems/System.h"
// #include "glm/gtc/bitfield.hpp"
// #include "Waldem/ECS/Components/Light.h"
// #include "Waldem/Renderer/Shader.h"
// #include "Waldem/Time.h"
// #include <glm/gtc/integer.hpp>
// #include "Waldem/ECS/Systems/DrawSystems/DrawSystem.h"
//
// #include "Waldem/ECS/Components/Ocean.h"
//
// namespace Waldem
// {
//     struct ButterflyPushConstants
//     {
//         uint32_t Stage;
//         uint32_t Pingpong;
//         int Direction;
//     };
//     
//     struct InversionConstantBuffer
//     {
//         int N;
//         uint32_t Pingpong;
//     };
//     
//     class WALDEM_API OceanSimulationSystem : public DrawSystem
//     {
//         //Gaussian Noise
//         Pipeline* GaussianNoisePipeline = nullptr;
//         RootSignature* GaussianNoiseRootSignature = nullptr;
//         ComputeShader* GaussianNoiseComputeShader = nullptr;
//         RenderTarget* GaussianNoiseRenderTarget = nullptr;
//
//         //Spectrum generation
//         Pipeline* SpectrumGenerationPipeline = nullptr;
//         RootSignature* SpectrumGenerationRootSignature = nullptr;
//         ComputeShader* SpectrumGenerationComputeShader = nullptr;
//         RenderTarget* H0 = nullptr;
//         RenderTarget* H0Inverse = nullptr;
//         RenderTarget* DxCoefficients = nullptr;
//         RenderTarget* DyCoefficients = nullptr;
//         RenderTarget* DzCoefficients = nullptr;
//         Point3 GroupCount;
//
//         //Butterfly texture generation
//         Pipeline* ButterflyTextureGenerationPipeline = nullptr;
//         RootSignature* ButterflyTextureGenerationRootSignature = nullptr;
//         ComputeShader* ButterflyTextureGenerationComputeShader = nullptr;
//         WArray<int> BitReversedIndices;
//         RenderTarget* ButterflyTexture512 = nullptr;
//         Point3 ButterflyTextureGenerationGroupCount;
//
//         //Butterfly
//         RenderTarget* DxPingPong = nullptr;
//         RenderTarget* DyPingPong = nullptr;
//         RenderTarget* DzPingPong = nullptr;
//         RenderTarget* Dx = nullptr;
//         RenderTarget* Dy = nullptr;
//         RenderTarget* Dz = nullptr;
//         Point3 ButterflyGroupCount;
//         
//         //Butterfly X
//         Pipeline* ButterflyXPipeline = nullptr;
//         RootSignature* ButterflyXRootSignature = nullptr;
//         ComputeShader* ButterflyXComputeShader = nullptr;
//         
//         //Butterfly Y
//         Pipeline* ButterflyYPipeline = nullptr;
//         RootSignature* ButterflyYRootSignature = nullptr;
//         ComputeShader* ButterflyYComputeShader = nullptr;
//         
//         //Butterfly Z
//         Pipeline* ButterflyZPipeline = nullptr;
//         RootSignature* ButterflyZRootSignature = nullptr;
//         ComputeShader* ButterflyZComputeShader = nullptr;
//         
//         //Inversion X
//         Pipeline* InversionXPipeline = nullptr;
//         RootSignature* InversionXRootSignature = nullptr;
//         ComputeShader* InversionXComputeShader = nullptr;
//         
//         //Inversion Y
//         Pipeline* InversionYPipeline = nullptr;
//         RootSignature* InversionYRootSignature = nullptr;
//         ComputeShader* InversionYComputeShader = nullptr;
//         
//         //Inversion Z
//         Pipeline* InversionZPipeline = nullptr;
//         RootSignature* InversionZRootSignature = nullptr;
//         ComputeShader* InversionZComputeShader = nullptr;
//         
//         WArray<ButterflyPushConstants> HorizontalPushConstants;
//         WArray<ButterflyPushConstants> VerticalPushConstants;
//         int Stages;
//
//         //Normal and displacement
//         Pipeline* NormalAndDisplacementPipeline = nullptr;
//         RootSignature* NormalAndDisplacementRootSignature = nullptr;
//         ComputeShader* NormalAndDisplacementComputeShader = nullptr;
//         RenderTarget* Normal = nullptr;
//         RenderTarget* Displacement = nullptr;
//
//         //Ocean displacement
//         Pipeline* OceanDisplacementPipeline = nullptr;
//         RootSignature* OceanDisplacementRootSignature = nullptr;
//         ComputeShader* OceanDisplacementComputeShader = nullptr;
//         WArray<Buffer*> VertexBuffers;
//         
//     public:
//         OceanSimulationSystem() : DrawSystem() {}
//         
//         unsigned int bitfieldReverse(unsigned int index , int numBits)
//         {
//             unsigned int reversed = 0;
//     
//             for (int i = 0; i < numBits; ++i)
//             {
//                 reversed <<= 1;
//
//                 reversed |= (index  & 1);
//
//                 index  >>= 1;
//             }
//
//             return reversed & ((1 << numBits) - 1);
//         }
//         
//         void Initialize(InputManager* inputManager, ResourceManager* resourceManager, CContentManager* contentManager) override
//         {
//             for (auto [entity, transform, meshComponent, ocean] : Manager->EntitiesWith<Transform, MeshComponent, Ocean>())
//             {
//                 GraphicResource oceanParametersCBResource = GraphicResource("OceanParameters", RTYPE_ConstantBuffer, &ocean, sizeof(Ocean), sizeof(Ocean), 0);
//                 
//                 VertexBuffers.Add(meshComponent.Mesh.Asset->VertexBuffer);
// 			    Point2 fftResolution = Point2(N, N);
//                 Stages = glm::log2(fftResolution.x);
//                 GaussianNoiseRenderTarget = resourceManager->CreateRenderTarget("GaussianNoise", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 H0 = resourceManager->CreateRenderTarget("H0", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 H0Inverse = resourceManager->CreateRenderTarget("H0Inverse", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 DxCoefficients = resourceManager->CreateRenderTarget("DxCoefficients", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 DyCoefficients = resourceManager->CreateRenderTarget("DyCoefficients", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 DzCoefficients = resourceManager->CreateRenderTarget("DzCoefficients", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 Normal = resourceManager->CreateRenderTarget("Normals", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 Displacement = resourceManager->CreateRenderTarget("Displacement", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 DxPingPong = resourceManager->CreateRenderTarget("DxPingPong", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 DyPingPong = resourceManager->CreateRenderTarget("DyPingPong", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 DzPingPong = resourceManager->CreateRenderTarget("DzPingPong", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 Dx = resourceManager->CreateRenderTarget("Dx", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 Dy = resourceManager->CreateRenderTarget("Dy", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 Dz = resourceManager->CreateRenderTarget("Dz", fftResolution.x, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 ButterflyTexture512 = resourceManager->CreateRenderTarget("ButterflyTexture512", Stages, fftResolution.y, TextureFormat::R32G32B32A32_FLOAT);
//                 
//                 //Gaussian noise initialization
//                 WArray<GraphicResource> resources;
//                 resources.Add(GraphicResource("GaussianNoiseRenderTarget", GaussianNoiseRenderTarget, 0, true));
//                 resources.Add(GraphicResource("MyPushConstants", RTYPE_Constant, nullptr, sizeof(float), sizeof(float), 0));
//                 GaussianNoiseRootSignature = Renderer::CreateRootSignature(resources);
//                 GaussianNoiseComputeShader = Renderer::LoadComputeShader("OceanSimulation/GaussianNoise");
//                 GaussianNoisePipeline = Renderer::CreateComputePipeline("GaussianNoisePipeline", GaussianNoiseRootSignature, GaussianNoiseComputeShader);
//                 resources.Clear();
//
//                 //Spectrum generation initialization
//                 resources.Add(oceanParametersCBResource);
//                 resources.Add(GraphicResource("MyPushConstants", RTYPE_Constant, nullptr, sizeof(float), sizeof(float), 1));
//                 resources.Add(GraphicResource("H0", H0, 0, true));
//                 resources.Add(GraphicResource("H0Inverse", H0Inverse, 1, true));
//                 resources.Add(GraphicResource("DxCoefficients", DxCoefficients, 2, true));
//                 resources.Add(GraphicResource("DyCoefficients", DyCoefficients, 3, true));
//                 resources.Add(GraphicResource("DzCoefficients", DzCoefficients, 4, true));
//                 resources.Add(GraphicResource("GaussianNoiseRenderTarget", GaussianNoiseRenderTarget, 0));
//                 SpectrumGenerationRootSignature = Renderer::CreateRootSignature(resources);
//                 SpectrumGenerationComputeShader = Renderer::LoadComputeShader("OceanSimulation/Spectrum");
//                 SpectrumGenerationPipeline = Renderer::CreateComputePipeline("SpectrumGenerationPipeline", SpectrumGenerationRootSignature, SpectrumGenerationComputeShader);
//                 resources.Clear();
//                 Point3 numThreads = Renderer::GetNumThreadsPerGroup(SpectrumGenerationComputeShader);
//                 GroupCount = Point3((fftResolution.x + numThreads.x - 1) / numThreads.x, (fftResolution.y + numThreads.y - 1) / numThreads.y, 1);
//
//                 //Butterfly texture generation initialization
//                 for (int i = 0; i < N; i++)
//                 {
//                     uint32_t x = bitfieldReverse(i, Stages);
//                     BitReversedIndices.Add(x);
//                 }
//                 resources.Add(GraphicResource("BitReversedIndices", RTYPE_Buffer, BitReversedIndices.GetData(), sizeof(int), BitReversedIndices.GetSize(), 0));
//                 resources.Add(GraphicResource("ButterflyTexture512", ButterflyTexture512, 0, true));
//                 resources.Add(GraphicResource("MyConstantBuffer", RTYPE_ConstantBuffer, &ocean.N, sizeof(uint32_t), sizeof(uint32_t), 0));
//                 ButterflyTextureGenerationRootSignature = Renderer::CreateRootSignature(resources);
//                 ButterflyTextureGenerationComputeShader = Renderer::LoadComputeShader("OceanSimulation/ButterflyTextureGeneration");
//                 ButterflyTextureGenerationPipeline = Renderer::CreateComputePipeline("ButterflyTextureGenerationPipeline", ButterflyTextureGenerationRootSignature, ButterflyTextureGenerationComputeShader);
//                 resources.Clear();
//                 ButterflyTextureGenerationGroupCount = Vector3(Stages, N/16, 1);
//
//                 //Butterfly Y initialization
//                 resources.Add(GraphicResource("ButterflyTexture512", ButterflyTexture512, 0));
//                 resources.Add(GraphicResource("DyCoefficients", DyCoefficients, 0, true));
//                 resources.Add(GraphicResource("DyPingPong", DyPingPong, 1, true));
//                 resources.Add(GraphicResource("MyPushConstants", RTYPE_Constant, nullptr, sizeof(uint32_t) * 3, sizeof(uint32_t) * 3, 0));
//                 ButterflyYRootSignature = Renderer::CreateRootSignature(resources);
//                 ButterflyYComputeShader = Renderer::LoadComputeShader("OceanSimulation/Butterfly");
//                 ButterflyYPipeline = Renderer::CreateComputePipeline("ButterflyYPipeline", ButterflyYRootSignature, ButterflyYComputeShader);
//                 resources.Clear();
//
//                 //Butterfly X initialization
//                 resources.Add(GraphicResource("ButterflyTexture512", ButterflyTexture512, 0));
//                 resources.Add(GraphicResource("DxCoefficients", DxCoefficients, 0, true));
//                 resources.Add(GraphicResource("DxPingPong", DxPingPong, 1, true));
//                 resources.Add(GraphicResource("MyPushConstants", RTYPE_Constant, nullptr, sizeof(uint32_t) * 3, sizeof(uint32_t) * 3, 0));
//                 ButterflyXRootSignature = Renderer::CreateRootSignature(resources);
//                 ButterflyXComputeShader = Renderer::LoadComputeShader("OceanSimulation/Butterfly");
//                 ButterflyXPipeline = Renderer::CreateComputePipeline("ButterflyXPipeline", ButterflyXRootSignature, ButterflyXComputeShader);
//                 resources.Clear();
//
//                 //Butterfly Z initialization
//                 resources.Add(GraphicResource("ButterflyTexture512", ButterflyTexture512, 0));
//                 resources.Add(GraphicResource("DzCoefficients", DzCoefficients, 0, true));
//                 resources.Add(GraphicResource("DzPingPong", DzPingPong, 1, true));
//                 resources.Add(GraphicResource("MyPushConstants", RTYPE_Constant, nullptr, sizeof(uint32_t) * 3, sizeof(uint32_t) * 3, 0));
//                 ButterflyZRootSignature = Renderer::CreateRootSignature(resources);
//                 ButterflyZComputeShader = Renderer::LoadComputeShader("OceanSimulation/Butterfly");
//                 ButterflyZPipeline = Renderer::CreateComputePipeline("ButterflyZPipeline", ButterflyZRootSignature, ButterflyZComputeShader);
//                 resources.Clear();
//
//                 ButterflyGroupCount = Vector3(N / 16, N / 16, 1);
//                 
//                 // Fill horizontal and vertical butterfly push constants
//                 uint32_t pingpong = 0;
//                 
//                 HorizontalPushConstants.Resize(Stages);
//                 for (uint32_t i = 0; i < Stages; i++)
//                 {
//                     ButterflyPushConstants& horizontalPushConstant = HorizontalPushConstants[i];
//                     horizontalPushConstant = { i, pingpong, 0 };
//                     
//                     pingpong++;
//                     pingpong %= 2;
//                 }
//                 
//                 VerticalPushConstants.Resize(Stages);
//                 for (uint32_t i = 0; i < Stages; i++)
//                 {
//                     ButterflyPushConstants& verticalPushConstant = VerticalPushConstants[i];
//                     verticalPushConstant = { i, pingpong, 1 };
//                     
//                     pingpong++;
//                     pingpong %= 2;
//                 }
//
//                 InversionConstantBuffer inversionConstantBuffer { N, pingpong };
//                 auto inversionConstantBufferResource = GraphicResource("MyConstantBuffer", RTYPE_ConstantBuffer, &inversionConstantBuffer, sizeof(InversionConstantBuffer), sizeof(InversionConstantBuffer), 0);
//
//                 //Inversion Y initialization
//                 resources.Add(GraphicResource("Dy", Dy, 0, true));
//                 resources.Add(GraphicResource("DyCoefficients", DyCoefficients, 0));
//                 resources.Add(GraphicResource("DyPingPong", DyPingPong, 1));
//                 resources.Add(inversionConstantBufferResource);
//                 InversionYRootSignature = Renderer::CreateRootSignature(resources);
//                 InversionYComputeShader = Renderer::LoadComputeShader("OceanSimulation/iFFT");
//                 InversionYPipeline = Renderer::CreateComputePipeline("InversionYPipeline", InversionYRootSignature, InversionYComputeShader);
//                 resources.Clear();
//                 
//                 //Inversion X initialization
//                 resources.Add(GraphicResource("Dx", Dx, 0, true));
//                 resources.Add(GraphicResource("DxCoefficients", DxCoefficients, 0));
//                 resources.Add(GraphicResource("DxPingPong", DxPingPong, 1));
//                 resources.Add(inversionConstantBufferResource);
//                 InversionXRootSignature = Renderer::CreateRootSignature(resources);
//                 InversionXComputeShader = Renderer::LoadComputeShader("OceanSimulation/iFFT");
//                 InversionXPipeline = Renderer::CreateComputePipeline("InversionXPipeline", InversionXRootSignature, InversionXComputeShader);
//                 resources.Clear();
//
//                 //Inversion Z initialization
//                 resources.Add(GraphicResource("Dz", Dz, 0, true));
//                 resources.Add(GraphicResource("DzCoefficients", DzCoefficients, 0));
//                 resources.Add(GraphicResource("DzPingPong", DzPingPong, 1));
//                 resources.Add(inversionConstantBufferResource);
//                 InversionZRootSignature = Renderer::CreateRootSignature(resources);
//                 InversionZComputeShader = Renderer::LoadComputeShader("OceanSimulation/iFFT");
//                 InversionZPipeline = Renderer::CreateComputePipeline("InversionZPipeline", InversionZRootSignature, InversionZComputeShader);
//                 resources.Clear();
//
//                 //Normal and displacement initialization
//                 resources.Add(GraphicResource("Dx", Dx, 0));
//                 resources.Add(GraphicResource("Dy", Dy, 1));
//                 resources.Add(GraphicResource("Dz", Dz, 2));
//                 resources.Add(GraphicResource("Normal", Normal, 0, true));
//                 resources.Add(GraphicResource("Displacement", Displacement, 1, true));
//                 resources.Add(oceanParametersCBResource);
//                 NormalAndDisplacementRootSignature = Renderer::CreateRootSignature(resources);
//                 NormalAndDisplacementComputeShader = Renderer::LoadComputeShader("OceanSimulation/NormalAndDisplacement");
//                 NormalAndDisplacementPipeline = Renderer::CreateComputePipeline("NormalAndDisplacementPipeline", NormalAndDisplacementRootSignature, NormalAndDisplacementComputeShader);
//                 resources.Clear();
//
//                 auto vertexBufferOriginal = resourceManager->CloneBuffer(VertexBuffers[0]);
//
//                 //Ocean displacement initialization
//                 resources.Add(oceanParametersCBResource);
//                 resources.Add(GraphicResource("Normal", Normal, 0));
//                 resources.Add(GraphicResource("Displacement", Displacement, 1));
//                 resources.Add(GraphicResource("VertexBufferOriginal", vertexBufferOriginal, 2));
//                 resources.Add(GraphicResource("VertexBuffers", VertexBuffers, 0, true));
//                 OceanDisplacementRootSignature = Renderer::CreateRootSignature(resources);
//                 OceanDisplacementComputeShader = Renderer::LoadComputeShader("OceanSimulation/OceanDisplacement");
//                 OceanDisplacementPipeline = Renderer::CreateComputePipeline("OceanDisplacementPipeline", OceanDisplacementRootSignature, OceanDisplacementComputeShader);
//                 resources.Clear();
//                 
//                 //Gaussian noise generation
//                 Renderer::ResourceBarrier(GaussianNoiseRenderTarget, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::SetPipeline(GaussianNoisePipeline);
//                 Renderer::SetRootSignature(GaussianNoiseRootSignature);
//                 float randomValue[2] = { static_cast<float>(rand()) / static_cast<float>(RAND_MAX), static_cast<float>(rand()) / static_cast<float>(RAND_MAX) };
//                 GaussianNoiseRootSignature->UpdateResourceData("MyPushConstants", &randomValue);
//                 Renderer::Compute(GroupCount);
//                 Renderer::ResourceBarrier(GaussianNoiseRenderTarget, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//
//                 //Butterfly texture generation
//                 Renderer::ResourceBarrier(ButterflyTexture512, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::SetPipeline(ButterflyTextureGenerationPipeline);
//                 Renderer::SetRootSignature(ButterflyTextureGenerationRootSignature);
//                 Renderer::Compute(ButterflyTextureGenerationGroupCount);
//                 Renderer::ResourceBarrier(ButterflyTexture512, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//             }
//         }
//     
//         void Update(float deltaTime) override
//         {
//             for (auto [entity, transform, meshComponent, ocean] : Manager->EntitiesWith<Transform, MeshComponent, Ocean>())
//             {
//                 //Spectrum generation
//                 //TODO: probably may be separated into initial spectrum generation and axes spectrum generation, and initial one could be moved to the initialization
//                 Renderer::ResourceBarrier(H0, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(H0Inverse, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(DxCoefficients, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(DyCoefficients, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(DzCoefficients, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::SetPipeline(SpectrumGenerationPipeline);
//                 Renderer::SetRootSignature(SpectrumGenerationRootSignature);
//                 SpectrumGenerationRootSignature->UpdateResourceData("OceanParameters", &ocean);
//                 SpectrumGenerationRootSignature->UpdateResourceData("MyPushConstants", &Time::ElapsedTime);
//                 Renderer::Compute(GroupCount);
//                 Renderer::ResourceBarrier(H0, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(H0Inverse, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//
//                 //Butterfly
//                 Renderer::ResourceBarrier(DxPingPong, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(DyPingPong, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(DzPingPong, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//
//                 // horizontal
//                 for (uint32_t i = 0; i < Stages; i++)
//                 {
//                     //Butterfly dy
//                     Renderer::SetPipeline(ButterflyYPipeline);
//                     Renderer::SetRootSignature(ButterflyYRootSignature);
//                     ButterflyYRootSignature->UpdateResourceData("MyPushConstants", &HorizontalPushConstants[i]);
//                     Renderer::Compute(ButterflyGroupCount);
//
//                     //Butterfly dx
//                     Renderer::SetPipeline(ButterflyXPipeline);
//                     Renderer::SetRootSignature(ButterflyXRootSignature);
//                     ButterflyXRootSignature->UpdateResourceData("MyPushConstants", &HorizontalPushConstants[i]);
//                     Renderer::Compute(ButterflyGroupCount);
//
//                     //Butterfly dz
//                     Renderer::SetPipeline(ButterflyZPipeline);
//                     Renderer::SetRootSignature(ButterflyZRootSignature);
//                     ButterflyZRootSignature->UpdateResourceData("MyPushConstants", &HorizontalPushConstants[i]);
//                     Renderer::Compute(ButterflyGroupCount);
//                 }
//
//                 // vertical
//                 for (uint32_t i = 0; i < Stages; i++)
//                 {
//                     //Butterfly dy
//                     Renderer::SetPipeline(ButterflyYPipeline);
//                     Renderer::SetRootSignature(ButterflyYRootSignature);
//                     ButterflyYRootSignature->UpdateResourceData("MyPushConstants", &VerticalPushConstants[i]);
//                     Renderer::Compute(ButterflyGroupCount);
//
//                     //Butterfly dx
//                     Renderer::SetPipeline(ButterflyXPipeline);
//                     Renderer::SetRootSignature(ButterflyXRootSignature);
//                     ButterflyXRootSignature->UpdateResourceData("MyPushConstants", &VerticalPushConstants[i]);
//                     Renderer::Compute(ButterflyGroupCount);
//
//                     //Butterfly dz
//                     Renderer::SetPipeline(ButterflyZPipeline);
//                     Renderer::SetRootSignature(ButterflyZRootSignature);
//                     ButterflyZRootSignature->UpdateResourceData("MyPushConstants", &VerticalPushConstants[i]);
//                     Renderer::Compute(ButterflyGroupCount);
//                 }
//                 
//                 Renderer::ResourceBarrier(DxCoefficients, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(DyCoefficients, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(DzCoefficients, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(DxPingPong, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(DyPingPong, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(DzPingPong, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//
//                 //Inversion
//                 Renderer::ResourceBarrier(Dx, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(Dy, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(Dz, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 
//                 //Inversion dy
//                 Renderer::SetPipeline(InversionYPipeline);
//                 Renderer::SetRootSignature(InversionYRootSignature);
//                 Renderer::Compute(ButterflyGroupCount);
//
//                 //Inversion dx
//                 Renderer::SetPipeline(InversionXPipeline);
//                 Renderer::SetRootSignature(InversionXRootSignature);
//                 Renderer::Compute(ButterflyGroupCount);
//
//                 //Inversion dz
//                 Renderer::SetPipeline(InversionZPipeline);
//                 Renderer::SetRootSignature(InversionZRootSignature);
//                 Renderer::Compute(ButterflyGroupCount);
//                 
//                 Renderer::ResourceBarrier(Dx, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(Dy, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(Dz, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//
//                 //Normal and displacement
//                 Renderer::ResourceBarrier(Normal, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::ResourceBarrier(Displacement, ALL_SHADER_RESOURCE, UNORDERED_ACCESS);
//                 Renderer::SetPipeline(NormalAndDisplacementPipeline);
//                 Renderer::SetRootSignature(NormalAndDisplacementRootSignature);
//                 NormalAndDisplacementRootSignature->UpdateResourceData("OceanParameters", &ocean);
//                 Renderer::Compute(ButterflyGroupCount);
//                 Renderer::ResourceBarrier(Normal, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//                 Renderer::ResourceBarrier(Displacement, UNORDERED_ACCESS, ALL_SHADER_RESOURCE);
//
//                 //Ocean displacement
//                 for (auto vertexBuffer : VertexBuffers)
//                 {
//                     Renderer::ResourceBarrier(vertexBuffer, ALL_SHADER_RESOURCE | VERTEX_AND_CONSTANT_BUFFER, UNORDERED_ACCESS);
//                 }
//                 Renderer::SetPipeline(OceanDisplacementPipeline);
//                 Renderer::SetRootSignature(OceanDisplacementRootSignature);
//                 OceanDisplacementRootSignature->UpdateResourceData("OceanParameters", &ocean);
//                 Renderer::Compute(ButterflyGroupCount);
//                 for (auto vertexBuffer : VertexBuffers)
//                 {
//                     Renderer::ResourceBarrier(vertexBuffer, UNORDERED_ACCESS, ALL_SHADER_RESOURCE | VERTEX_AND_CONSTANT_BUFFER);
//                 }
//             }
//             
//         }
//
//         void OnResize(Vector2 size) override
//         {
//             
//         }
//     };
// }
