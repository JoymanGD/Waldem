#pragma once
#include "Waldem/ECS/IdManager.h"
#include "Waldem/Input/Input.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/SkeletalMeshComponent.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Model/Quad.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Sky.h"
#include "Waldem/ECS/Components/Sprite.h"
#include "Waldem/Renderer/ResizableAccelerationStructure.h"
#include "Waldem/Renderer/ResizableBuffer.h"
#include "Waldem/Renderer/Viewport/Viewport.h"

#define MAX_INDIRECT_COMMANDS 500

namespace Waldem
{
    struct SGBufferSceneData
    {
        Matrix4 ViewMatrix;
        Matrix4 ProjectionMatrix;
        Matrix4 WorldMatrix;
        Matrix4 InverseProjectionMatrix;
    };

    struct GBufferRootConstants
    {
        uint WorldTransforms;
        uint MaterialAttributes;
        uint SceneDataBuffer;
    };
    
    struct SkinningRootConstants
    {
        uint BindPoseVertexBuffer;
        uint SkinnedVertexBuffer;
        uint VertexBonesBuffer;
        uint BoneMatricesBuffer;
        uint VertexOffset;
        uint VertexCount;
    };

    struct SkySceneData
    {
        Matrix4 InverseProjection;
        Matrix4 InverseView;
        Matrix4 ViewProjection;
        Vector4 SkyZenithColor;
        Vector4 SkyHorizonColor;
        Vector4 GroundColor;
        Vector4 SunDirection;
        Vector4 CameraPosition;
    };

    struct SkyRootConstants
    {
        uint SceneDataBuffer;
    };
    
    class WALDEM_API GBufferRenderingSystem : public ISystem
    {
        inline static GBufferRenderingSystem* ActiveInstance = nullptr;

        //Sky pass
        Pipeline* SkyPipeline = nullptr;
        PixelShader* SkyPixelShader = nullptr;
        Quad FullscreenQuad = {};
        SkyRootConstants SkyPassRootConstants;
        SkySceneData SkyPassSceneData;
        Buffer* SceneDataBuffer = nullptr;
        
        //GBuffer pass
        Pipeline* BFCGBufferPipeline = nullptr; // Back face culling GBuffer pipeline
        Pipeline* NCGBufferPipeline = nullptr; // No culling GBuffer pipeline
        PixelShader* GBufferPixelShader = nullptr;
        ResizableBuffer DrawCommandsBuffer;
        ResizableBuffer BFCIndirectBuffer; //Back face culling indirect buffer
        WArray<IndirectIndexedCommand> BFCIndirectCommands;
        ResizableBuffer NCIndirectBuffer; //No culling indirect buffer
        WArray<IndirectIndexedCommand> NCIndirectCommands;
        ResizableBuffer WorldTransformsBuffer;
        ResizableBuffer MaterialAttributesBuffer;
        Buffer* GBufferSceneDataBuffer = nullptr;
        GBufferRootConstants GBufferRootConstants;
        SGBufferSceneData GBufferSceneData;
        WArray<MaterialShaderAttribute> MaterialAttributes;
        size_t VerticesCount = 0;
        size_t IndicesCount = 0;

        WArray<AccelerationStructure*> BLAS;
        WMap<StaticMesh*, AccelerationStructure*> BLASToUpdate;
        ResizableBuffer LightsBuffer;
        ResizableBuffer LightTransformsBuffer;
        ResizableBuffer LightsIndicesBuffer;
        WArray<int> LightsIndices;
        WArray<Matrix4> LightTransforms;

        //Skeletal mesh skinning
        Pipeline* SkinningPipeline = nullptr;
        ComputeShader* SkinningComputeShader = nullptr;
        SkinningRootConstants SkinningConstants;

        struct SkeletalSkinningEntry
        {
            uint BindPoseVertexSRV;
            uint VertexBonesSRV;
            Buffer* BoneMatricesBuffer;
            int VertexOffset;
            uint VertexCount;
            DrawIndexedCommand DrawCommand;
            int GlobalDrawId;
        };
        WMap<flecs::entity_t, SkeletalSkinningEntry> SkeletalSkinningData;

        //Sprite rendering
        WArray<Vertex> SpriteVertices;
        WArray<uint32> SpriteIndices;
        Buffer* SpriteVertexBuffer;
        Buffer* SpriteIndexBuffer;
        
    public:
        GBufferRenderingSystem()
        {
            ActiveInstance = this;
            SpriteVertices =
            {
                { {Vector4(-0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(0,1)} },
                { {Vector4( 0.5f, -0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(1,1)} },
                { {Vector4( 0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(1,0)} },
                { {Vector4(-0.5f,  0.5f, 0, 1)}, {Vector4(1,1,1,1)}, {Vector4(0,0,1,0)}, {Vector4(0,1,0,0)}, {Vector4(1,0,0,0)}, {Vector2(0,0)} },
            };
            SpriteIndices = { 0,1,2, 2,3,0 };

            SpriteVertexBuffer = Renderer::CreateBuffer("SpriteVertexBuffer", BufferType::VertexBuffer, SpriteVertices.GetSize(), sizeof(Vertex), SpriteVertices.GetData());
            SpriteIndexBuffer = Renderer::CreateBuffer("SpriteIndexBuffer", BufferType::IndexBuffer, SpriteIndices.GetSize(), sizeof(uint32_t), SpriteIndices.GetData());
        }

        static void ResetSceneRuntimeData()
        {
            if(ActiveInstance == nullptr)
            {
                return;
            }

            auto& instance = *ActiveInstance;

            instance.BFCIndirectCommands.Clear();
            instance.NCIndirectCommands.Clear();
            instance.MaterialAttributes.Clear();
            instance.LightsIndices.Clear();
            instance.LightTransforms.Clear();
            instance.SkeletalSkinningData.Clear();

            instance.VerticesCount = 0;
            instance.IndicesCount = 0;
            instance.DrawCommandsBuffer.Size = 0;
            instance.BFCIndirectBuffer.Size = 0;
            instance.NCIndirectBuffer.Size = 0;
            instance.WorldTransformsBuffer.Size = 0;
            instance.MaterialAttributesBuffer.Size = 0;
            instance.LightsBuffer.Size = 0;
            instance.LightTransformsBuffer.Size = 0;
            instance.LightsIndicesBuffer.Size = 0;

            Renderer::RenderData.VertexBuffer.Size = 0;
            Renderer::RenderData.IndexBuffer.Size = 0;
        }
        
        void Initialize() override
        {
            AlwaysActive = true;
            
            //Sky
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 12, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };
            
            SceneDataBuffer = Renderer::CreateBuffer("SkySceneDataBuffer", BufferType::StorageBuffer, sizeof(SkyPassSceneData), sizeof(SkyPassSceneData));
            SkyPassRootConstants.SceneDataBuffer = SceneDataBuffer->GetIndex(SRV_CBV);
            
            SkyPixelShader = Renderer::LoadPixelShader("Sky");
            SkyPipeline = Renderer::CreateGraphicPipeline("SkyPipeline",
                                                            SkyPixelShader,
                                                            { SGBuffer::GetFormat(SkyColor) },
                                                            TextureFormat::UNKNOWN,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            inputElementDescs);
            
            //GBuffer
            DrawCommandsBuffer = ResizableBuffer("DrawCommandsBuffer", BufferType::StorageBuffer, sizeof(DrawIndexedCommand), MAX_INDIRECT_COMMANDS);
            BFCIndirectBuffer = ResizableBuffer("BFCIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectIndexedCommand), MAX_INDIRECT_COMMANDS);
            NCIndirectBuffer = ResizableBuffer("NCIndirectBuffer", BufferType::IndirectBuffer, sizeof(IndirectIndexedCommand), MAX_INDIRECT_COMMANDS);
            Renderer::RenderData.VertexBuffer = ResizableBuffer("VertexBuffer", BufferType::VertexBuffer, sizeof(Vertex), 1000);
            Renderer::RenderData.IndexBuffer = ResizableBuffer("IndexBuffer", BufferType::IndexBuffer, sizeof(uint), 1000);
            WorldTransformsBuffer = ResizableBuffer("WorldTransformsBuffer", BufferType::StorageBuffer, sizeof(Matrix4), MAX_INDIRECT_COMMANDS);
            MaterialAttributesBuffer = ResizableBuffer("MaterialAttributesBuffer", BufferType::StorageBuffer, sizeof(MaterialShaderAttribute), MAX_INDIRECT_COMMANDS);
            GBufferSceneDataBuffer = Renderer::CreateBuffer("SceneDataBuffer", BufferType::StorageBuffer, sizeof(SGBufferSceneData), sizeof(SGBufferSceneData));
            
            GBufferPixelShader = Renderer::LoadPixelShader("GBuffer");
            BFCGBufferPipeline = Renderer::CreateGraphicPipeline("BFCGBufferPipeline",
                                                            GBufferPixelShader,
                                                            { SGBuffer::GetFormat(WorldPosition), SGBuffer::GetFormat(Normal), SGBuffer::GetFormat(Color), SGBuffer::GetFormat(ORM), SGBuffer::GetFormat(MeshID) },
                                                            TextureFormat::D32_FLOAT,
                                                            DEFAULT_RASTERIZER_DESC,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            RasterizerDesc spriteRasterizer = DEFAULT_RASTERIZER_DESC;
            spriteRasterizer.CullMode = WD_CULL_MODE_NONE;
            NCGBufferPipeline = Renderer::CreateGraphicPipeline("NCGBufferPipeline",
                                                            GBufferPixelShader,
                                                            { SGBuffer::GetFormat(WorldPosition), SGBuffer::GetFormat(Normal), SGBuffer::GetFormat(Color), SGBuffer::GetFormat(ORM), SGBuffer::GetFormat(MeshID) },
                                                            TextureFormat::D32_FLOAT,
                                                            spriteRasterizer,
                                                            DEFAULT_DEPTH_STENCIL_DESC,
                                                            DEFAULT_BLEND_DESC,
                                                            WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                                            DEFAULT_INPUT_LAYOUT_DESC);

            LightsBuffer = ResizableBuffer("LightsBuffer", StorageBuffer, sizeof(Light), 40);
            LightTransformsBuffer = ResizableBuffer("LightTransformsBuffer", StorageBuffer, sizeof(Matrix4), 40);
            LightsIndicesBuffer = ResizableBuffer("LightsIndicesBuffer", StorageBuffer, sizeof(int), 40);
            Renderer::RenderData.TLAS = ResizableAccelerationStructure("RayTracingTLAS", 50);
            
            //Skeletal mesh skinning
            SkinningComputeShader = Renderer::LoadComputeShader("Animation");
            SkinningPipeline = Renderer::CreateComputePipeline("SkinningPipeline", SkinningComputeShader);

            auto clearIndirectCommand = [&](ResizableBuffer& buffer, WArray<IndirectIndexedCommand>& commands, int drawId)
            {
                if(drawId < 0 || drawId >= commands.Num())
                {
                    return;
                }

                auto& command = commands[drawId];
                command.DrawId = -1;
                command.Command = { 0, 0, 0, 0, 0 };
                buffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * drawId);
            };

            auto ensureIndirectSlot = [&](ResizableBuffer& buffer, WArray<IndirectIndexedCommand>& commands, int drawId)
            {
                if(drawId < 0)
                {
                    return;
                }

                if(drawId >= commands.Num())
                {
                    commands.Resize(drawId + 1);
                    for(int index = 0; index <= drawId; ++index)
                    {
                        if(commands[index].DrawId == 0 && commands[index].Command.IndexCountPerInstance == 0)
                        {
                            commands[index].DrawId = -1;
                        }
                    }
                }

                buffer.UpdateOrAdd(nullptr, sizeof(IndirectIndexedCommand), drawId * sizeof(IndirectIndexedCommand));
            };
            
            ECS::World.observer<MeshComponent, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, MeshComponent& meshComponent, Transform& transform)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                auto bfcDrawId = IdManager::AddId(entity, BackFaceCullingDrawIdType);
                auto ncDrawId = IdManager::AddId(entity, NoCullingDrawIdType);
                ensureIndirectSlot(BFCIndirectBuffer, BFCIndirectCommands, bfcDrawId);
                ensureIndirectSlot(NCIndirectBuffer, NCIndirectCommands, ncDrawId);
                clearIndirectCommand(BFCIndirectBuffer, BFCIndirectCommands, bfcDrawId);
                clearIndirectCommand(NCIndirectBuffer, NCIndirectCommands, ncDrawId);
                WorldTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4)); 

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.AddData(nullptr, sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.AddData(nullptr, sizeof(DrawIndexedCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData(globalDrawId);
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnSet).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    if(meshComponent.MeshRef.IsEmpty() && !meshComponent.MeshRef.IsValid())
                    {
                        return;
                    }

                    if(!meshComponent.MeshRef.IsEmpty() && !meshComponent.MeshRef.IsValid())
                    {
                        meshComponent.MeshRef.LoadAsset();
                    }
                    
                    if(meshComponent.MeshRef.IsValid())
                    {
                        if(meshComponent.MaterialRef.Reference.empty() || meshComponent.MaterialRef.Reference == "Empty")
                        {
                            meshComponent.MaterialRef.Reference = meshComponent.MeshRef.Mesh->MaterialPath;
                            meshComponent.MaterialRef.IsLoaded = false;
                        }

                        if(!meshComponent.MaterialRef.IsLoaded)
                        {
                            meshComponent.MaterialRef.LoadAsset();
                        }

                        Material* activeMaterial = meshComponent.MaterialRef.Mat;
                        const bool noCulling = activeMaterial != nullptr && activeMaterial->TwoSided;
                        int bfcDrawId = -1;
                        int ncDrawId = -1;
                        if(!IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId) || !IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                        {
                            return;
                        }
                        uint vertexCount = meshComponent.MeshRef.Mesh->VertexData.Num();

                        const bool isFirstAssignment = (meshComponent.DrawCommand.IndexCountPerInstance == 0);

                        if(isFirstAssignment)
                        {
                            meshComponent.DrawCommand = {
                                (uint)meshComponent.MeshRef.Mesh->IndexData.Num(),
                                1,
                                (uint)IndicesCount,
                                (int)VerticesCount,
                                0
                            };

                            Renderer::RenderData.VertexBuffer.AddData(meshComponent.MeshRef.Mesh->VertexData.GetData(), meshComponent.MeshRef.Mesh->VertexData.GetSize());
                            Renderer::RenderData.IndexBuffer.AddData(meshComponent.MeshRef.Mesh->IndexData.GetData(), meshComponent.MeshRef.Mesh->IndexData.GetSize());

                            VerticesCount += vertexCount;
                            IndicesCount += meshComponent.DrawCommand.IndexCountPerInstance;

                            DrawCommandsBuffer.UpdateData(&meshComponent.DrawCommand, sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                            auto& transform = entity.get<Transform>();
                            Renderer::Wait();
                            Renderer::RenderData.TLAS.SetData(globalDrawId, meshComponent.MeshRef.Mesh->Name, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, meshComponent.DrawCommand, vertexCount, transform);
                        }

                        IndirectIndexedCommand command;
                        command.DrawId = globalDrawId;
                        command.Command = meshComponent.DrawCommand;

                        if(noCulling)
                        {
                            clearIndirectCommand(BFCIndirectBuffer, BFCIndirectCommands, bfcDrawId);
                            NCIndirectCommands[ncDrawId] = command;
                            NCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * ncDrawId);
                        }
                        else
                        {
                            clearIndirectCommand(NCIndirectBuffer, NCIndirectCommands, ncDrawId);
                            BFCIndirectCommands[bfcDrawId] = command;
                            BFCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * bfcDrawId);
                        }

                        auto& materialAttribute = MaterialAttributes[globalDrawId];

                        if(activeMaterial)
                        {
                            materialAttribute.Albedo = activeMaterial->Albedo;
                            materialAttribute.Metallic = activeMaterial->Metallic;
                            materialAttribute.Roughness = activeMaterial->Roughness;
                            materialAttribute.AlphaCut = activeMaterial->AlphaCut ? 1 : 0;
                            materialAttribute.CastShadows = activeMaterial->CastShadows ? 1 : 0;
                        }
                        materialAttribute.DiffuseTextureID = -1;
                        materialAttribute.NormalTextureID = -1;
                        materialAttribute.ORMTextureID = -1;

                        if(activeMaterial)
                        {
                            if(activeMaterial->HasDiffuseTexture())
                                materialAttribute.DiffuseTextureID = activeMaterial->GetDiffuseTexture()->GetIndex(SRV_CBV);
                            if(activeMaterial->HasNormalTexture())
                                materialAttribute.NormalTextureID = activeMaterial->GetNormalTexture()->GetIndex(SRV_CBV);
                            if(activeMaterial->HasORMTexture())
                                materialAttribute.ORMTextureID = activeMaterial->GetORMTexture()->GetIndex(SRV_CBV);
                        }

                        MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                    }
                }
            });
            
            ECS::World.observer<MeshComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, MeshComponent& meshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    IndirectIndexedCommand command = {};
                    bool hasCommand = false;

                    int bfcDrawId;
                    if(IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId))
                    {
                        command = BFCIndirectCommands[bfcDrawId];
                        hasCommand = true;
                        BFCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), bfcDrawId * sizeof(IndirectIndexedCommand));
                        BFCIndirectCommands[bfcDrawId].DrawId = -1;
                        BFCIndirectCommands[bfcDrawId].Command = { 0, 0, 0, 0, 0 };
                        IdManager::RemoveId(entity, BackFaceCullingDrawIdType);
                    }

                    int ncDrawId;
                    if(IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                    {
                        command = NCIndirectCommands[ncDrawId];
                        hasCommand = true;
                        NCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), ncDrawId * sizeof(IndirectIndexedCommand));
                        NCIndirectCommands[ncDrawId].DrawId = -1;
                        NCIndirectCommands[ncDrawId].Command = { 0, 0, 0, 0, 0 };
                        IdManager::RemoveId(entity, NoCullingDrawIdType);
                    }

                    if(hasCommand)
                    {
                        if(meshComponent.MeshRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(meshComponent.MeshRef.Mesh->VertexData.GetSize(), command.Command.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(meshComponent.MeshRef.Mesh->IndexData.GetSize(), command.Command.StartIndexLocation * sizeof(uint));

                            VerticesCount -= meshComponent.MeshRef.Mesh->VertexData.Num();
                            IndicesCount -= meshComponent.MeshRef.Mesh->IndexData.Num();
                        }

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);
                        
                        if(entity.has<Transform>())
                        {
                            WorldTransformsBuffer.RemoveData(sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        }
                    }

                    IdManager::RemoveId(entity, GlobalDrawIdType);
                }
            });

            ECS::World.observer<SkeletalMeshComponent, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, SkeletalMeshComponent& skeletalMeshComponent, Transform& transform)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                auto bfcDrawId = IdManager::AddId(entity, BackFaceCullingDrawIdType);
                auto ncDrawId = IdManager::AddId(entity, NoCullingDrawIdType);
                ensureIndirectSlot(BFCIndirectBuffer, BFCIndirectCommands, bfcDrawId);
                ensureIndirectSlot(NCIndirectBuffer, NCIndirectCommands, ncDrawId);
                clearIndirectCommand(BFCIndirectBuffer, BFCIndirectCommands, bfcDrawId);
                clearIndirectCommand(NCIndirectBuffer, NCIndirectCommands, ncDrawId);

                WorldTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.AddData(nullptr, sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.AddData(nullptr, sizeof(DrawIndexedCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData(globalDrawId);
            });

            ECS::World.observer<SkeletalMeshComponent>().event(flecs::OnSet).each([&](flecs::entity entity, SkeletalMeshComponent& skeletalMeshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    bool meshReferenceIsEmpty = skeletalMeshComponent.MeshRef.Reference.empty() || skeletalMeshComponent.MeshRef.Reference == "Empty";

                    if(meshReferenceIsEmpty && !skeletalMeshComponent.MeshRef.IsValid())
                    {
                        return;
                    }

                    if(!meshReferenceIsEmpty && !skeletalMeshComponent.MeshRef.IsValid())
                    {
                        skeletalMeshComponent.MeshRef.LoadAsset();
                    }

                    if(skeletalMeshComponent.MeshRef.IsValid())
                    {
                        if(skeletalMeshComponent.MaterialRef.Reference != skeletalMeshComponent.MeshRef.Mesh->MaterialPath || !skeletalMeshComponent.MaterialRef.IsLoaded)
                        {
                            skeletalMeshComponent.MaterialRef.Reference = skeletalMeshComponent.MeshRef.Mesh->MaterialPath;
                            skeletalMeshComponent.MaterialRef.LoadAsset();
                        }

                        Material* activeMaterial = skeletalMeshComponent.MaterialRef.Mat;
                        const bool noCulling = activeMaterial != nullptr && activeMaterial->TwoSided;
                        int bfcDrawId = -1;
                        int ncDrawId = -1;
                        if(!IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId) || !IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                        {
                            return;
                        }
                        SkeletalMesh* skeletalMesh = skeletalMeshComponent.MeshRef.Mesh;

                        skeletalMeshComponent.DrawCommand.IndexCountPerInstance = (uint)skeletalMesh->IndexData.Num();
                        skeletalMeshComponent.DrawCommand.InstanceCount         = 1;
                        skeletalMeshComponent.DrawCommand.StartInstanceLocation = 0;

                        uint vertexCount = skeletalMesh->VertexData.Num();

                        // Only pack vertex/index data on first assignment; re-assignments update in-place
                        bool isFirstAssignment = SkeletalSkinningData.Find(entity.id()) == nullptr;

                        if(isFirstAssignment)
                        {
                            skeletalMeshComponent.DrawCommand.StartIndexLocation  = (uint)IndicesCount;
                            skeletalMeshComponent.DrawCommand.BaseVertexLocation  = (int)VerticesCount;

                            Renderer::RenderData.VertexBuffer.AddData(skeletalMesh->VertexData.GetData(), skeletalMesh->VertexData.GetSize());
                            Renderer::RenderData.IndexBuffer.AddData(skeletalMesh->IndexData.GetData(), skeletalMesh->IndexData.GetSize());

                            VerticesCount += vertexCount;
                            IndicesCount  += skeletalMeshComponent.DrawCommand.IndexCountPerInstance;
                        }

                        IndirectIndexedCommand command;
                        command.DrawId = globalDrawId;
                        command.Command = skeletalMeshComponent.DrawCommand;

                        if(noCulling)
                        {
                            clearIndirectCommand(BFCIndirectBuffer, BFCIndirectCommands, bfcDrawId);
                            NCIndirectCommands[ncDrawId] = command;
                            NCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * ncDrawId);
                        }
                        else
                        {
                            clearIndirectCommand(NCIndirectBuffer, NCIndirectCommands, ncDrawId);
                            BFCIndirectCommands[bfcDrawId] = command;
                            BFCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * bfcDrawId);
                        }

                        auto& materialAttribute = MaterialAttributes[globalDrawId];

                        if(activeMaterial)
                        {
                            materialAttribute.Albedo = activeMaterial->Albedo;
                            materialAttribute.Metallic = activeMaterial->Metallic;
                            materialAttribute.Roughness = activeMaterial->Roughness;
                            materialAttribute.AlphaCut = activeMaterial->AlphaCut ? 1 : 0;
                            materialAttribute.CastShadows = activeMaterial->CastShadows ? 1 : 0;
                        }
                        materialAttribute.DiffuseTextureID = -1;
                        materialAttribute.NormalTextureID = -1;
                        materialAttribute.ORMTextureID = -1;

                        if(activeMaterial)
                        {
                            if(activeMaterial->HasDiffuseTexture())
                                materialAttribute.DiffuseTextureID = activeMaterial->GetDiffuseTexture()->GetIndex(SRV_CBV);
                            if(activeMaterial->HasNormalTexture())
                                materialAttribute.NormalTextureID = activeMaterial->GetNormalTexture()->GetIndex(SRV_CBV);
                            if(activeMaterial->HasORMTexture())
                                materialAttribute.ORMTextureID = activeMaterial->GetORMTexture()->GetIndex(SRV_CBV);
                        }

                        MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.UpdateData(&skeletalMeshComponent.DrawCommand, sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                        auto& transform = entity.get<Transform>();

                        // Clean up previous bone matrices buffer if this entity was already registered
                        // (use the map as source of truth, not the component field, to avoid stale pointer issues)
                        SkeletalSkinningEntry* existingEntry = SkeletalSkinningData.Find(entity.id());
                        if(existingEntry && existingEntry->BoneMatricesBuffer)
                        {
                            Renderer::Destroy(existingEntry->BoneMatricesBuffer);
                            delete existingEntry->BoneMatricesBuffer;
                            existingEntry->BoneMatricesBuffer = nullptr;
                            skeletalMeshComponent.BoneMatricesBuffer = nullptr;
                        }

                        int boneCount = skeletalMesh->BoneCount > 0 ? skeletalMesh->BoneCount : 1;
                        skeletalMeshComponent.BoneCount = boneCount;

                        WArray<Matrix4> identityMatrices;
                        identityMatrices.Resize(boneCount, Matrix4(1.0f));
                        skeletalMeshComponent.BoneMatricesBuffer = Renderer::CreateBuffer(
                            "BoneMatricesBuffer",
                            StorageBuffer,
                            boneCount * sizeof(Matrix4),
                            sizeof(Matrix4),
                            identityMatrices.GetData()
                        );

                        // Register skinning data for the per-frame skinning compute dispatch
                        SkeletalSkinningEntry& entry = SkeletalSkinningData[entity.id()];
                        entry.BindPoseVertexSRV  = skeletalMesh->VertexBuffer->GetIndex(SRV_CBV);
                        entry.VertexBonesSRV     = skeletalMesh->VertexBonesBuffer->GetIndex(SRV_CBV);
                        entry.BoneMatricesBuffer = skeletalMeshComponent.BoneMatricesBuffer;
                        entry.VertexOffset       = skeletalMeshComponent.DrawCommand.BaseVertexLocation;
                        entry.VertexCount        = vertexCount;
                        entry.DrawCommand        = skeletalMeshComponent.DrawCommand;
                        entry.GlobalDrawId       = globalDrawId;

                        Renderer::Wait();
                        Renderer::RenderData.TLAS.SetData(globalDrawId, skeletalMesh->Name, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, skeletalMeshComponent.DrawCommand, vertexCount, transform);
                    }
                }
            });

            ECS::World.observer<SkeletalMeshComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, SkeletalMeshComponent& skeletalMeshComponent)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    IndirectIndexedCommand command = {};
                    bool hasCommand = false;

                    int bfcDrawId;
                    if(IdManager::GetId(entity, BackFaceCullingDrawIdType, bfcDrawId))
                    {
                        command = BFCIndirectCommands[bfcDrawId];
                        hasCommand = true;
                        BFCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), bfcDrawId * sizeof(IndirectIndexedCommand));
                        BFCIndirectCommands[bfcDrawId].DrawId  = -1;
                        BFCIndirectCommands[bfcDrawId].Command = { 0, 0, 0, 0, 0 };
                        IdManager::RemoveId(entity, BackFaceCullingDrawIdType);
                    }

                    int ncDrawId;
                    if(IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                    {
                        command = NCIndirectCommands[ncDrawId];
                        hasCommand = true;
                        NCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), ncDrawId * sizeof(IndirectIndexedCommand));
                        NCIndirectCommands[ncDrawId].DrawId  = -1;
                        NCIndirectCommands[ncDrawId].Command = { 0, 0, 0, 0, 0 };
                        IdManager::RemoveId(entity, NoCullingDrawIdType);
                    }

                    if(hasCommand)
                    {
                        if(skeletalMeshComponent.MeshRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(skeletalMeshComponent.MeshRef.Mesh->VertexData.GetSize(), command.Command.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(skeletalMeshComponent.MeshRef.Mesh->IndexData.GetSize(), command.Command.StartIndexLocation * sizeof(uint));

                            VerticesCount -= skeletalMeshComponent.MeshRef.Mesh->VertexData.Num();
                            IndicesCount  -= skeletalMeshComponent.MeshRef.Mesh->IndexData.Num();
                        }

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);

                        if(entity.has<Transform>())
                        {
                            WorldTransformsBuffer.RemoveData(sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        }
                    }

                    if(skeletalMeshComponent.BoneMatricesBuffer)
                    {
                        Renderer::Destroy(skeletalMeshComponent.BoneMatricesBuffer);
                        delete skeletalMeshComponent.BoneMatricesBuffer;
                        skeletalMeshComponent.BoneMatricesBuffer = nullptr;
                    }

                    SkeletalSkinningData.Remove(entity.id());

                    IdManager::RemoveId(entity, GlobalDrawIdType);
                }
            });

            ECS::World.observer<Sprite, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Sprite& sprite, Transform& transform)
            {
                auto globalDrawId = IdManager::AddId(entity, GlobalDrawIdType);
                auto ncDrawId = IdManager::AddId(entity, NoCullingDrawIdType);
                
                if(ncDrawId >= NCIndirectCommands.Num())
                {
                    NCIndirectCommands.Add(IndirectIndexedCommand());
                    NCIndirectBuffer.UpdateOrAdd(nullptr, sizeof(IndirectIndexedCommand), ncDrawId * sizeof(IndirectIndexedCommand));
                }
                
                WorldTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));

                if(globalDrawId >= MaterialAttributes.Num())
                {
                    MaterialAttributes.Add(MaterialShaderAttribute());
                    MaterialAttributesBuffer.UpdateOrAdd(nullptr, sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                    DrawCommandsBuffer.UpdateOrAdd(nullptr, sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));
                }

                Renderer::RenderData.TLAS.AddEmptyData(globalDrawId);
            });

            ECS::World.observer<Sprite>().event(flecs::OnSet).each([&](flecs::entity entity, Sprite& sprite)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int ncDrawId;
                    
                    if(IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                    {
                        bool textureReferenceIsEmpty = sprite.TextureRef.Reference.empty() || sprite.TextureRef.Reference == "Empty";
                        
                        if(textureReferenceIsEmpty && !sprite.TextureRef.IsValid())
                        {
                            return;
                        }

                        if(!textureReferenceIsEmpty && !sprite.TextureRef.IsValid())
                        {
                            sprite.TextureRef.LoadAsset();
                        }
                        
                        if(sprite.TextureRef.IsValid())
                        {
                            sprite.DrawCommand = {
                                (uint)SpriteIndices.Num(),
                                1,
                                (uint)IndicesCount,
                                (int)VerticesCount,
                                0
                            };
                            auto& command = NCIndirectCommands[ncDrawId];
                            command.DrawId = globalDrawId;
                            command.Command = sprite.DrawCommand;

                            NCIndirectBuffer.UpdateData(&command, sizeof(IndirectIndexedCommand), sizeof(IndirectIndexedCommand) * ncDrawId);

                            Renderer::RenderData.VertexBuffer.AddData(SpriteVertices.GetData(), SpriteVertices.GetSize());
                            Renderer::RenderData.IndexBuffer.AddData(SpriteIndices.GetData(), SpriteIndices.GetSize());

                            VerticesCount += SpriteVertices.Num();
                            IndicesCount += SpriteIndices.Num();
                            
                            auto& materialAttribute = MaterialAttributes[globalDrawId];

                            materialAttribute.Albedo = sprite.Color;
                            materialAttribute.AlphaCut = sprite.AlphaCut ? 1 : 0;
                            materialAttribute.DiffuseTextureID = sprite.TextureRef.Texture->GetIndex(SRV_CBV);

                            MaterialAttributesBuffer.UpdateData(&materialAttribute, sizeof(MaterialShaderAttribute), sizeof(MaterialShaderAttribute) * globalDrawId);
                            DrawCommandsBuffer.UpdateData(&sprite.DrawCommand, sizeof(DrawIndexedCommand), sizeof(DrawIndexedCommand) * globalDrawId);
                        }

                        auto transform = entity.get<Transform>();

                        WString spriteName = "Sprite_";
                        spriteName += sprite.TextureRef.Texture->GetName(); 

                        Renderer::RenderData.TLAS.SetData(globalDrawId, spriteName, Renderer::RenderData.VertexBuffer, Renderer::RenderData.IndexBuffer, sprite.DrawCommand, SpriteVertices.Num(), transform);
                    }
                }
            });

            ECS::World.observer<Sprite>().event(flecs::OnRemove).each([&](flecs::entity entity, Sprite& sprite)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    int ncDrawId;
                    if(IdManager::GetId(entity, NoCullingDrawIdType, ncDrawId))
                    {
                        IndirectIndexedCommand& command = NCIndirectCommands[ncDrawId];

                        if(sprite.TextureRef.IsValid())
                        {
                            Renderer::RenderData.VertexBuffer.RemoveData(SpriteVertices.GetSize(), command.Command.BaseVertexLocation * sizeof(Vertex));
                            Renderer::RenderData.IndexBuffer.RemoveData(SpriteIndices.GetSize(), command.Command.StartIndexLocation * sizeof(uint));

                            VerticesCount -= SpriteVertices.Num();
                            IndicesCount -= SpriteIndices.Num();
                        }
                        
                        NCIndirectBuffer.RemoveData(sizeof(IndirectIndexedCommand), ncDrawId * sizeof(IndirectIndexedCommand));
                        
                        command.DrawId = -1;
                        command.Command = { 0, 0, 0, 0, 0 };

                        MaterialAttributesBuffer.RemoveData(sizeof(MaterialShaderAttribute), globalDrawId * sizeof(MaterialShaderAttribute));
                        DrawCommandsBuffer.RemoveData(sizeof(DrawIndexedCommand), globalDrawId * sizeof(DrawIndexedCommand));

                        Renderer::RenderData.TLAS.RemoveData(globalDrawId);
                        
                        if(entity.has<Transform>())
                        {
                            WorldTransformsBuffer.RemoveData(sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        }

                        IdManager::RemoveId(entity, NoCullingDrawIdType);
                    }

                    IdManager::RemoveId(entity, GlobalDrawIdType);
                }
            });
            
            ECS::World.observer<Transform>().event(flecs::OnSet).each([&](flecs::entity entity, Transform& transform)
            {
                int globalDrawId;

                if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                {
                    WorldTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                    
                    Renderer::RenderData.TLAS.UpdateTransform(globalDrawId, transform);
                }
                
                if(entity.has<Light>())
                {
                    int lightId;

                    if(IdManager::GetId(entity, LightIdType, lightId))
                    {
                        LightTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    }
                }
            });
            
            ECS::World.observer<Light, Transform>().event(flecs::OnAdd).each([&](flecs::entity entity, Light& light, Transform& transform)
            {
                auto lightId = IdManager::AddId(entity, LightIdType);

                LightsBuffer.UpdateOrAdd(&light, sizeof(Light), sizeof(Light) * lightId);
                LightTransformsBuffer.UpdateOrAdd(&transform.RenderMatrix, sizeof(Matrix4), sizeof(Matrix4) * lightId);
                LightsIndices.Add(lightId);
                LightsIndicesBuffer.UpdateOrAdd(LightsIndices.GetData(), LightsIndices.GetSize(), 0);
                
            });

            Observer<Transform>("HybridRenderingTransformSyncOnSet", flecs::OnSet, [&](flecs::entity entity, Transform& transform)
            {
                if(entity.has<MeshComponent>() || entity.has<Light>() || entity.has<Sprite>())
                {
                    int globalDrawId;
                    if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
                    {
                        WorldTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), globalDrawId * sizeof(Matrix4));
                        Renderer::RenderData.TLAS.UpdateTransform(globalDrawId, transform);
                    }
                }

                if(entity.has<Light>())
                {
                    int lightId;
                    if(IdManager::GetId(entity, LightIdType, lightId))
                    {
                        LightTransformsBuffer.UpdateData(&transform.RenderMatrix, sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    }
                }
            });
            
            ECS::World.observer<Light>().event(flecs::OnSet).each([&](flecs::entity entity, Light& light)
            {
                int lightId;

                if(IdManager::GetId(entity, LightIdType, lightId))
                {
                    LightsBuffer.UpdateData(&light, sizeof(Light), sizeof(Light) * lightId);
                }
            });
            
            ECS::World.observer<Light>().event(flecs::OnRemove).each([&](flecs::entity entity, Light& light)
            {
                int lightId;

                if(IdManager::GetId(entity, LightIdType, lightId))
                {
                    LightTransformsBuffer.RemoveData(sizeof(Matrix4), sizeof(Matrix4) * lightId);
                    LightsBuffer.RemoveData(sizeof(Light), sizeof(Light) * lightId);
                    LightsIndices.Remove(lightId);
                    LightsIndicesBuffer.UpdateData(LightsIndices.GetData(), LightsIndices.GetSize(), 0);
                    IdManager::RemoveId(entity, LightIdType);
                }
            });

            ECS::World.observer<Sky>().event(flecs::OnSet).yield_existing().each([&](Sky& skybox)
            {
                SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
            });

            ECS::World.system("SkyColorClearingSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                
                viewport->GetGBuffer()->Clear({SkyColor});
            });
            
            ECS::World.system<Sky>("SkyRenderingSystem").kind<ECS::OnDraw>().each([&](Sky& skybox)
            {
                if(!Renderer::RenderData.FeatureToggles.EnableSkyPass)
                {
                    return;
                }

                auto q = ECS::World.query<Light, Transform>();

                bool hasSun = false;
                Vector3 sunDirection = Vector3(0, -1, 0);
                
                q.each([&hasSun, &sunDirection](ECS::Entity e, Light& light, Transform& transform)
                {
                    if (light.Type == LightType::Directional)
                    {
                        hasSun = true;
                        
                        sunDirection = -transform.GetForwardVector();

                        return;
                    }
                });

                if (!hasSun)
                {
                    return;
                }

                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    auto skyColor = viewport->GetGBufferRenderTarget(SkyColor);
                    if(!linkedCamera.is_alive() || !linkedCamera.has<Camera>() || !linkedCamera.has<Transform>())
                    {
                        return;
                    }
                    auto& cameraComponent = linkedCamera.get<Camera>();
                    auto& transformComponent = linkedCamera.get<Transform>();

                    SkyPassSceneData.InverseProjection = inverse(cameraComponent.ProjectionMatrix);
                    SkyPassSceneData.InverseView = transformComponent.RenderMatrix;
                    SkyPassSceneData.ViewProjection = cameraComponent.ProjectionMatrix * inverse(transformComponent.RenderMatrix);
                    SkyPassSceneData.SkyZenithColor = Vector4(skybox.SkyZenithColor, 1.0f);
                    SkyPassSceneData.SkyHorizonColor = Vector4(skybox.SkyHorizonColor, 1.0f);
                    SkyPassSceneData.GroundColor = Vector4(skybox.GroundColor, 1.0f);
                    SkyPassSceneData.SunDirection = Vector4(sunDirection, 1.0f);
                    SkyPassSceneData.CameraPosition = Vector4(transformComponent.RenderPosition, 1.0f);
                    
                    Renderer::UploadBuffer(SceneDataBuffer, &SkyPassSceneData, sizeof(SkySceneData));
                    Renderer::ResourceBarrier(skyColor, ALL_SHADER_RESOURCE, RENDER_TARGET);
                    Renderer::BindRenderTargets(skyColor);
                    Renderer::BindDepthStencil(nullptr);
                    Renderer::SetPipeline(SkyPipeline);
                    Renderer::PushConstants(&SkyPassRootConstants, sizeof(SkyRootConstants));
                    Renderer::Draw(&FullscreenQuad);
                    Renderer::ResourceBarrier(skyColor, RENDER_TARGET, ALL_SHADER_RESOURCE);
                }
            });
            
            ECS::World.system("SkeletalMeshSkinningSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                if(SkeletalSkinningData.IsEmpty())
                    return;

                Renderer::ResourceBarrier(Renderer::RenderData.VertexBuffer.GetBuffer(), UNORDERED_ACCESS);

                for(uint skinIdx = 0; skinIdx < SkeletalSkinningData.Num(); ++skinIdx)
                {
                    auto& entry = SkeletalSkinningData.At(skinIdx).value;

                    SkinningConstants.BindPoseVertexBuffer = entry.BindPoseVertexSRV;
                    SkinningConstants.SkinnedVertexBuffer = Renderer::RenderData.VertexBuffer.GetIndex(UAV);
                    SkinningConstants.VertexBonesBuffer = entry.VertexBonesSRV;
                    SkinningConstants.BoneMatricesBuffer = entry.BoneMatricesBuffer->GetIndex(SRV_CBV);
                    SkinningConstants.VertexOffset = (uint)entry.VertexOffset;
                    SkinningConstants.VertexCount = entry.VertexCount;

                    Renderer::SetPipeline(SkinningPipeline);
                    Renderer::PushConstants(&SkinningConstants, sizeof(SkinningRootConstants));

                    uint groupCount = (entry.VertexCount + 63) / 64;
                    Renderer::Compute(Point3(groupCount, 1, 1));
                }

                Renderer::UAVBarrier(Renderer::RenderData.VertexBuffer.GetBuffer());
                Renderer::ResourceBarrier(Renderer::RenderData.VertexBuffer.GetBuffer(), (ResourceStates)(VERTEX_AND_CONSTANT_BUFFER | NON_PIXEL_SHADER_RESOURCE));

                for(uint skinIdx = 0; skinIdx < SkeletalSkinningData.Num(); ++skinIdx)
                {
                    auto& entry = SkeletalSkinningData.At(skinIdx).value;

                    Renderer::RenderData.TLAS.UpdateGeometry(entry.GlobalDrawId, Renderer::RenderData.VertexBuffer.GetBuffer(), Renderer::RenderData.IndexBuffer.GetBuffer(), entry.DrawCommand, entry.VertexCount);
                }
            });

            ECS::World.system("GBufferClearSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                SViewport* viewport = Renderer::GetCurrentViewport();
                viewport->GetGBuffer()->Clear({WorldPosition, Normal, Color, ORM, MeshID, Depth});
            });
            
            ECS::World.system("GBufferSystem").kind<ECS::OnDraw>().run([&](flecs::iter& it)
            {
                // Publish shared scene buffers for offline training systems.
                Renderer::RenderData.SharedDrawCommandsBuffer = DrawCommandsBuffer.GetBuffer();
                Renderer::RenderData.SharedMaterialAttributesBuffer = MaterialAttributesBuffer.GetBuffer();
                Renderer::RenderData.SharedWorldTransformsBuffer = WorldTransformsBuffer.GetBuffer();
                Renderer::RenderData.SharedLightsBuffer = LightsBuffer.GetBuffer();
                Renderer::RenderData.SharedLightTransformsBuffer = LightTransformsBuffer.GetBuffer();
                Renderer::RenderData.SharedLightsIndicesBuffer = LightsIndicesBuffer.GetBuffer();
                Renderer::RenderData.SharedDrawCommandsCount = static_cast<uint>(BFCIndirectCommands.Num() + NCIndirectCommands.Num());
                Renderer::RenderData.SharedNumLights = static_cast<uint>(LightsIndices.Num());

                if(!Renderer::RenderData.FeatureToggles.EnableGBufferPass)
                {
                    return;
                }

                if(BFCIndirectCommands.Num() <= 0 && NCIndirectCommands.Num() <= 0)
                {
                    return;
                }
                
                auto viewport = Renderer::GetCurrentViewport();
                
                ECS::Entity linkedCamera;
                
                if(viewport->TryGetLinkedCamera(linkedCamera))
                {
                    if(!linkedCamera.is_alive() || !linkedCamera.has<Camera>() || !linkedCamera.has<Transform>())
                    {
                        return;
                    }
                    auto& cameraComponent = linkedCamera.get<Camera>();
                    auto& transformComponent = linkedCamera.get<Transform>();
                    
                    GBufferSceneData.ProjectionMatrix = cameraComponent.ProjectionMatrix;
                    GBufferSceneData.ViewMatrix = inverse(transformComponent.RenderMatrix);
                    GBufferSceneData.WorldMatrix = transformComponent.RenderMatrix;
                    GBufferSceneData.InverseProjectionMatrix = inverse(cameraComponent.ProjectionMatrix);
                    
                    Renderer::UploadBuffer(GBufferSceneDataBuffer, &GBufferSceneData, sizeof(SGBufferSceneData));

                    auto gbuffer = viewport->GetGBuffer();

                    GBufferRootConstants.WorldTransforms = WorldTransformsBuffer.GetIndex(SRV_CBV);
                    GBufferRootConstants.MaterialAttributes = MaterialAttributesBuffer.GetIndex(SRV_CBV);
                    GBufferRootConstants.SceneDataBuffer = GBufferSceneDataBuffer->GetIndex(SRV_CBV);

                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, ALL_SHADER_RESOURCE, RENDER_TARGET);
                    gbuffer->Barrier(Depth, ALL_SHADER_RESOURCE, DEPTH_WRITE);
                    
                    // Back face culling pass
                    if(BFCIndirectCommands.Num() > 0)
                    {
                        Renderer::SetPipeline(BFCGBufferPipeline);
                        Renderer::PushConstants(&GBufferRootConstants, sizeof(GBufferRootConstants));
                        Renderer::BindRenderTargets({ gbuffer->GetRenderTarget(WorldPosition), gbuffer->GetRenderTarget(Normal), gbuffer->GetRenderTarget(Color), gbuffer->GetRenderTarget(ORM), gbuffer->GetRenderTarget(MeshID) });
                        Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                        Renderer::SetVertexBuffers(Renderer::RenderData.VertexBuffer.GetBuffer(), 1);
                        Renderer::SetIndexBuffer(Renderer::RenderData.IndexBuffer.GetBuffer());
                        Renderer::DrawIndirect(BFCIndirectCommands.Num(), BFCIndirectBuffer);
                    }
                    
                    // No culling pass
                    if(NCIndirectCommands.Num() > 0)
                    {
                        Renderer::SetPipeline(NCGBufferPipeline);
                        Renderer::PushConstants(&GBufferRootConstants, sizeof(GBufferRootConstants));
                        Renderer::BindRenderTargets({ gbuffer->GetRenderTarget(WorldPosition), gbuffer->GetRenderTarget(Normal), gbuffer->GetRenderTarget(Color), gbuffer->GetRenderTarget(ORM), gbuffer->GetRenderTarget(MeshID) });
                        Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                        Renderer::SetVertexBuffers(Renderer::RenderData.VertexBuffer.GetBuffer(), 1);
                        Renderer::SetIndexBuffer(Renderer::RenderData.IndexBuffer.GetBuffer());
                        Renderer::DrawIndirect(NCIndirectCommands.Num(), NCIndirectBuffer);
                    }
                    
                    gbuffer->Barriers({WorldPosition, Normal, Color, ORM, MeshID}, RENDER_TARGET, ALL_SHADER_RESOURCE);
                    gbuffer->Barrier(Depth, DEPTH_WRITE, ALL_SHADER_RESOURCE);
                }
            });

        }
    };
}
