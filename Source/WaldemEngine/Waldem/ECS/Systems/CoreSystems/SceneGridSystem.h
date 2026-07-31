#pragma once

#include <algorithm>
#include <vector>

#include "glm/gtc/matrix_transform.hpp"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/ECS/Components/Light.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/SceneGridComponent.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Systems/System.h"
#include "Waldem/Renderer/Model/MeshBase.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Shader.h"
#include "Waldem/Renderer/Viewport/Viewport.h"

namespace Waldem
{
    class SceneGridSystem : public ISystem
    {
        struct GridInstanceData
        {
            Matrix4 WorldMatrix = Matrix4(1.0f);
            Vector4 Color = Vector4(1.0f);
        };

        struct GridSceneData
        {
            Matrix4 ViewProjection = Matrix4(1.0f);
        };

        struct GridRootConstants
        {
            uint InstanceBufferID = 0;
            uint SceneDataBufferID = 0;
        };

        struct GridRenderData
        {
            Buffer* InstanceBuffer = nullptr;
            uint32 InstanceBufferCapacity = 0;
            uint32 InstanceCount = 0;
            bool Dirty = true;
        };

        Pipeline* GridPipeline = nullptr;
        PixelShader* GridPixelShader = nullptr;
        Buffer* GridVertexBuffer = nullptr;
        Buffer* GridIndexBuffer = nullptr;
        Buffer* SceneDataBuffer = nullptr;
        uint GridIndexCount = 0;
        GridSceneData SceneData = {};
        bool AllGridsDirty = true;
        WMap<ECS::Entity, GridRenderData> GridMeshes;

        static int ClampDimension(float value)
        {
            return glm::max(1, (int)glm::round(value));
        }

        static int ClampSubdivisionLevels(float value)
        {
            return glm::max(0, (int)glm::round(value));
        }

        static int ClampSubdivisionFactor(float value)
        {
            return glm::max(2, (int)glm::round(value));
        }

        static bool TryGetMeshBounds(ECS::Entity entity, Transform& transform, MeshComponent& meshComponent, AABB& outBounds)
        {
            if(meshComponent.MeshRef.IsValid() && meshComponent.MeshRef.Mesh)
            {
                outBounds = meshComponent.MeshRef.Mesh->BBox;
                if(!transform.HasBoundingBox)
                {
                    transform.BoundingBox = outBounds;
                    transform.HasBoundingBox = true;
                }
                return true;
            }

            if(transform.HasBoundingBox)
            {
                outBounds = transform.BoundingBox;
                return true;
            }

            return false;
        }

        static bool TryGetLightBounds(Transform& transform, Light& light, AABB& outBounds)
        {
            switch(light.Type)
            {
            case LightType::Point:
            case LightType::Spot:
                {
                    const Vector3 extent(light.Radius);
                    outBounds = AABB(transform.Position - extent, transform.Position + extent);
                    return true;
                }
            case LightType::Area:
                {
                    const Vector3 extent(light.AreaWidth * 0.5f, light.AreaHeight * 0.5f, 0.1f);
                    outBounds = AABB(transform.Position - extent, transform.Position + extent);
                    return true;
                }
            case LightType::Directional:
            default:
                return false;
            }
        }

        static AABB GetGridLocalBounds(const SceneGridComponent& grid)
        {
            const Vector3 dimensions(
                (float)ClampDimension(grid.GridDimensions.x),
                (float)ClampDimension(grid.GridDimensions.y),
                (float)ClampDimension(grid.GridDimensions.z)
            );

            const Vector3 size = dimensions * grid.CellSize;
            const Vector3 min = grid.CenterOnTransform ? grid.LocalOffset - size * 0.5f : grid.LocalOffset;
            return AABB(min, min + size);
        }

        void EnsureRenderDataEntry(ECS::Entity entity)
        {
            if(!GridMeshes.Contains(entity))
            {
                GridMeshes[entity] = {};
            }
        }

        void MarkGridDirty(ECS::Entity entity)
        {
            EnsureRenderDataEntry(entity);
            GridMeshes[entity].Dirty = true;
        }

        void MarkAllGridDataDirty()
        {
            AllGridsDirty = true;
            for(auto& pair : GridMeshes)
            {
                pair.value.Dirty = true;
            }
        }

        void EnsureInstanceBuffer(ECS::Entity entity, uint32 instanceCount)
        {
            EnsureRenderDataEntry(entity);
            auto& renderData = GridMeshes[entity];

            const uint32 requiredSize = glm::max<uint32>((uint32)sizeof(GridInstanceData), instanceCount * (uint32)sizeof(GridInstanceData));
            if(renderData.InstanceBuffer && renderData.InstanceBufferCapacity == requiredSize)
            {
                return;
            }

            if(renderData.InstanceBuffer)
            {
                Renderer::Destroy(renderData.InstanceBuffer);
            }

            renderData.InstanceBuffer = Renderer::CreateBuffer(
                "SceneGridInstances_" + std::to_string((uint64_t)entity.id()),
                BufferType::StorageBuffer,
                requiredSize,
                sizeof(GridInstanceData)
            );
            renderData.InstanceBufferCapacity = requiredSize;
        }

        void ReleaseGridBuffer(ECS::Entity entity)
        {
            if(!GridMeshes.Contains(entity))
            {
                return;
            }

            if(GridMeshes[entity].InstanceBuffer)
            {
                Renderer::Destroy(GridMeshes[entity].InstanceBuffer);
            }
            GridMeshes.Remove(entity);
        }

        static Matrix4 BuildCellWorldMatrix(const AABB& cellBounds, const Matrix4& gridWorldMatrix)
        {
            const Vector3 center = (cellBounds.Min + cellBounds.Max) * 0.5f;
            const Vector3 size = cellBounds.Max - cellBounds.Min;

            Matrix4 localMatrix(1.0f);
            localMatrix = glm::translate(localMatrix, center);
            localMatrix = glm::scale(localMatrix, size);
            return gridWorldMatrix * localMatrix;
        }

        static AABB GetCellBoundsAtDepth(const SceneGridComponent& grid, const SceneGridCellData& cell)
        {
            const AABB gridLocalBounds = GetGridLocalBounds(grid);
            const int subdivisionFactor = ClampSubdivisionFactor(grid.SubdivisionFactor);
            float scaleDivisor = 1.0f;
            for(int depth = 0; depth < cell.Depth; ++depth)
            {
                scaleDivisor *= (float)subdivisionFactor;
            }

            const float cellSizeAtDepth = grid.CellSize / scaleDivisor;
            const Vector3 cellMin = gridLocalBounds.Min + Vector3(cell.Position) * cellSizeAtDepth;
            return AABB(cellMin, cellMin + Vector3(cellSizeAtDepth));
        }

        static void AddCellInstance(const AABB& cellBounds, const Matrix4& gridWorldMatrix, const Vector4& color, WArray<GridInstanceData>& targetInstances)
        {
            GridInstanceData instanceData;
            instanceData.WorldMatrix = BuildCellWorldMatrix(cellBounds, gridWorldMatrix);
            instanceData.Color = color;
            targetInstances.Add(instanceData);
        }

        static int GetCellType(const std::vector<AABB>& intersectingMeshBounds, const std::vector<AABB>& intersectingLightBounds)
        {
            if(!intersectingLightBounds.empty())
            {
                return SceneGridCell_Light;
            }

            if(!intersectingMeshBounds.empty())
            {
                return SceneGridCell_Mesh;
            }

            return SceneGridCell_Empty;
        }

        void AppendSubdividedCellData(
            const SceneGridComponent& grid,
            const AABB& cellBounds,
            const Point3& cellPosition,
            int cellDepth,
            const std::vector<AABB>& intersectingMeshBounds,
            const std::vector<AABB>& intersectingLightBounds,
            const Matrix4& gridWorldMatrix,
            bool visualizeOnlyOccupiedCells,
            int remainingLevels,
            int subdivisionFactor,
            WArray<SceneGridCellData>& cells,
            WArray<GridInstanceData>& freeInstances,
            WArray<GridInstanceData>& occupiedInstances)
        {
            const int cellType = GetCellType(intersectingMeshBounds, intersectingLightBounds);
            if(cellType == SceneGridCell_Empty)
            {
                SceneGridCellData cellData;
                cellData.Position = cellPosition;
                cellData.Depth = cellDepth;
                cellData.Type = SceneGridCell_Empty;
                cells.Add(cellData);

                if(!visualizeOnlyOccupiedCells)
                {
                    AddCellInstance(cellBounds, gridWorldMatrix, Vector4(0.0f, 1.0f, 0.0f, 1.0f), freeInstances);
                }
                return;
            }

            if(remainingLevels <= 0)
            {
                SceneGridCellData cellData;
                cellData.Position = cellPosition;
                cellData.Depth = cellDepth;
                cellData.Type = cellType;
                cells.Add(cellData);

                const Vector4 color = cellType == SceneGridCell_Light
                    ? Vector4(1.0f, 1.0f, 0.0f, 1.0f)
                    : Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                AddCellInstance(cellBounds, gridWorldMatrix, color, occupiedInstances);
                return;
            }

            const Vector3 childSize = (cellBounds.Max - cellBounds.Min) / (float)subdivisionFactor;
            for(int z = 0; z < subdivisionFactor; ++z)
            {
                for(int y = 0; y < subdivisionFactor; ++y)
                {
                    for(int x = 0; x < subdivisionFactor; ++x)
                    {
                        const Vector3 childMin = cellBounds.Min + Vector3((float)x, (float)y, (float)z) * childSize;
                        const AABB childBounds(childMin, childMin + childSize);
                        std::vector<AABB> childIntersectingMeshBounds;
                        childIntersectingMeshBounds.reserve(intersectingMeshBounds.size());
                        std::vector<AABB> childIntersectingLightBounds;
                        childIntersectingLightBounds.reserve(intersectingLightBounds.size());

                        for(const AABB& meshBounds : intersectingMeshBounds)
                        {
                            if(childBounds.Intersects(meshBounds))
                            {
                                childIntersectingMeshBounds.push_back(meshBounds);
                            }
                        }

                        for(const AABB& lightBounds : intersectingLightBounds)
                        {
                            if(childBounds.Intersects(lightBounds))
                            {
                                childIntersectingLightBounds.push_back(lightBounds);
                            }
                        }

                        AppendSubdividedCellData(
                            grid,
                            childBounds,
                            cellPosition * subdivisionFactor + Point3(x, y, z),
                            cellDepth + 1,
                            childIntersectingMeshBounds,
                            childIntersectingLightBounds,
                            gridWorldMatrix,
                            visualizeOnlyOccupiedCells,
                            remainingLevels - 1,
                            subdivisionFactor,
                            cells,
                            freeInstances,
                            occupiedInstances
                        );
                    }
                }
            }
        }

        void BuildGridData(ECS::Entity gridEntity, Transform& gridTransform, SceneGridComponent& grid, WArray<GridInstanceData>& instances)
        {
            if(grid.CellSize <= 0.0f || !grid.Visualize)
            {
                grid.Cells.Clear();
                return;
            }

            const int dimX = ClampDimension(grid.GridDimensions.x);
            const int dimY = ClampDimension(grid.GridDimensions.y);
            const int dimZ = ClampDimension(grid.GridDimensions.z);
            const AABB gridLocalBounds = GetGridLocalBounds(grid);
            const Matrix4 gridWorldMatrix = gridTransform.RenderMatrix;
            const Matrix4 inverseGridWorldMatrix = inverse(gridWorldMatrix);
            const Vector3 gridCenterLocal = (gridLocalBounds.Min + gridLocalBounds.Max) * 0.5f;
            const bool visualizeOnlyOccupiedCells = grid.VisualizeOnlyOccupiedCells;
            const bool subdivideOccupiedCells = grid.SubdivideOccupiedCells;
            const int subdivisionLevels = ClampSubdivisionLevels(grid.SubdivisionLevels);
            const int subdivisionFactor = ClampSubdivisionFactor(grid.SubdivisionFactor);
            std::vector<AABB> meshBoundsInGridSpace;
            std::vector<AABB> lightBoundsInGridSpace;

            grid.GridData.CenterPosition = Vector3(gridWorldMatrix * Vector4(gridCenterLocal, 1.0f));
            grid.GridData.Size = Point3(dimX, dimY, dimZ);
            grid.GridData.CellSize = (int)glm::round(grid.CellSize);
            grid.GridData.SubdivisionFactor = subdivisionFactor;
            grid.Cells.Clear();

            auto meshQuery = ECS::World.query_builder<Transform, MeshComponent>().build();
            meshQuery.each([&](flecs::entity entity, Transform& transform, MeshComponent& meshComponent)
            {
                if(entity.id() == gridEntity.id())
                {
                    return;
                }

                AABB meshLocalBounds;
                if(!TryGetMeshBounds(entity, transform, meshComponent, meshLocalBounds))
                {
                    return;
                }

                const AABB meshGridBounds = meshLocalBounds.GetTransformed(transform.RenderMatrix).GetTransformed(inverseGridWorldMatrix);
                if(meshGridBounds.Max.x <= gridLocalBounds.Min.x || meshGridBounds.Min.x >= gridLocalBounds.Max.x ||
                   meshGridBounds.Max.y <= gridLocalBounds.Min.y || meshGridBounds.Min.y >= gridLocalBounds.Max.y ||
                   meshGridBounds.Max.z <= gridLocalBounds.Min.z || meshGridBounds.Min.z >= gridLocalBounds.Max.z)
                {
                    return;
                }

                meshBoundsInGridSpace.push_back(meshGridBounds);
            });

            auto lightQuery = ECS::World.query_builder<Transform, Light>().build();
            lightQuery.each([&](flecs::entity entity, Transform& transform, Light& light)
            {
                if(entity.id() == gridEntity.id())
                {
                    return;
                }

                AABB lightWorldBounds;
                if(!TryGetLightBounds(transform, light, lightWorldBounds))
                {
                    return;
                }

                const AABB lightGridBounds = lightWorldBounds.GetTransformed(inverseGridWorldMatrix);
                if(lightGridBounds.Max.x <= gridLocalBounds.Min.x || lightGridBounds.Min.x >= gridLocalBounds.Max.x ||
                   lightGridBounds.Max.y <= gridLocalBounds.Min.y || lightGridBounds.Min.y >= gridLocalBounds.Max.y ||
                   lightGridBounds.Max.z <= gridLocalBounds.Min.z || lightGridBounds.Min.z >= gridLocalBounds.Max.z)
                {
                    return;
                }

                lightBoundsInGridSpace.push_back(lightGridBounds);
            });

            WArray<GridInstanceData> freeInstances;
            WArray<GridInstanceData> occupiedInstances;
            for(int z = 0; z < dimZ; ++z)
            {
                for(int y = 0; y < dimY; ++y)
                {
                    for(int x = 0; x < dimX; ++x)
                    {
                        const Vector3 cellMin = gridLocalBounds.Min + Vector3((float)x, (float)y, (float)z) * grid.CellSize;
                        const AABB cellBounds(cellMin, cellMin + Vector3(grid.CellSize));
                        std::vector<AABB> intersectingMeshBounds;
                        intersectingMeshBounds.reserve(meshBoundsInGridSpace.size());
                        std::vector<AABB> intersectingLightBounds;
                        intersectingLightBounds.reserve(lightBoundsInGridSpace.size());

                        for(const AABB& meshBounds : meshBoundsInGridSpace)
                        {
                            if(cellBounds.Intersects(meshBounds))
                            {
                                intersectingMeshBounds.push_back(meshBounds);
                            }
                        }

                        for(const AABB& lightBounds : lightBoundsInGridSpace)
                        {
                            if(cellBounds.Intersects(lightBounds))
                            {
                                intersectingLightBounds.push_back(lightBounds);
                            }
                        }

                        const int cellType = GetCellType(intersectingMeshBounds, intersectingLightBounds);
                        if(cellType == SceneGridCell_Empty)
                        {
                            SceneGridCellData cellData;
                            cellData.Position = Point3(x, y, z);
                            cellData.Depth = 0;
                            cellData.Type = SceneGridCell_Empty;
                            grid.Cells.Add(cellData);

                            if(!visualizeOnlyOccupiedCells)
                            {
                                AddCellInstance(cellBounds, gridWorldMatrix, Vector4(0.0f, 1.0f, 0.0f, 1.0f), freeInstances);
                            }
                            continue;
                        }

                        if(subdivideOccupiedCells && subdivisionLevels > 0)
                        {
                            AppendSubdividedCellData(
                                grid,
                                cellBounds,
                                Point3(x, y, z),
                                0,
                                intersectingMeshBounds,
                                intersectingLightBounds,
                                gridWorldMatrix,
                                visualizeOnlyOccupiedCells,
                                subdivisionLevels,
                                subdivisionFactor,
                                grid.Cells,
                                freeInstances,
                                occupiedInstances
                            );
                        }
                        else
                        {
                            SceneGridCellData cellData;
                            cellData.Position = Point3(x, y, z);
                            cellData.Depth = 0;
                            cellData.Type = cellType;
                            grid.Cells.Add(cellData);

                            const Vector4 color = cellType == SceneGridCell_Light
                                ? Vector4(1.0f, 1.0f, 0.0f, 1.0f)
                                : Vector4(1.0f, 0.0f, 0.0f, 1.0f);
                            AddCellInstance(cellBounds, gridWorldMatrix, color, occupiedInstances);
                        }
                    }
                }
            }

            instances.AddRange(freeInstances);
            instances.AddRange(occupiedInstances);
        }

        void RebuildGridInstances(ECS::Entity entity, Transform& transform, SceneGridComponent& grid)
        {
            WArray<GridInstanceData> instances;
            BuildGridData(entity, transform, grid, instances);
            if(instances.IsEmpty())
            {
                ReleaseGridBuffer(entity);
                return;
            }

            EnsureInstanceBuffer(entity, (uint32)instances.Num());
            auto& renderData = GridMeshes[entity];
            Renderer::UploadBuffer(renderData.InstanceBuffer, instances.GetData(), (uint32)instances.GetSize());
            renderData.InstanceCount = (uint32)instances.Num();
            renderData.Dirty = false;
        }

        void CreateSharedGridMesh()
        {
            WArray<Vertex> vertices =
            {
                { { -0.5f, -0.5f, -0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
                { {  0.5f, -0.5f, -0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
                { {  0.5f,  0.5f, -0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
                { { -0.5f,  0.5f, -0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
                { { -0.5f, -0.5f,  0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
                { {  0.5f, -0.5f,  0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
                { {  0.5f,  0.5f,  0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
                { { -0.5f,  0.5f,  0.5f, 1.0f }, { 1, 1, 1, 1 }, {}, {}, {}, {} },
            };

            WArray<uint32> indices =
            {
                0, 1, 1, 2, 2, 3, 3, 0,
                4, 5, 5, 6, 6, 7, 7, 4,
                0, 4, 1, 5, 2, 6, 3, 7
            };

            GridIndexCount = (uint)indices.Num();
            GridVertexBuffer = Renderer::CreateBuffer("SceneGridUnitCubeVB", BufferType::VertexBuffer, (uint32)vertices.GetSize(), sizeof(Vertex), vertices.GetData());
            GridIndexBuffer = Renderer::CreateBuffer("SceneGridUnitCubeIB", BufferType::IndexBuffer, (uint32)indices.GetSize(), sizeof(uint32), indices.GetData());
        }

    public:
        void Initialize(InputManager* inputManager) override
        {
            WArray<InputLayoutDesc> inputElementDescs = {
                { "POSITION", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 16, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            DepthStencilDesc depthStencilDesc = DEFAULT_DEPTH_STENCIL_DESC;
            depthStencilDesc.DepthEnable = false;
            depthStencilDesc.DepthWriteMask = WD_DEPTH_WRITE_MASK_ZERO;

            RasterizerDesc rasterizerDesc = DEFAULT_RASTERIZER_DESC;
            rasterizerDesc.CullMode = WD_CULL_MODE_NONE;

            SceneDataBuffer = Renderer::CreateBuffer("SceneGridSceneData", BufferType::StorageBuffer, sizeof(GridSceneData), sizeof(GridSceneData));
            CreateSharedGridMesh();

            GridPixelShader = Renderer::LoadPixelShader("GridInstanced");
            GridPipeline = Renderer::CreateGraphicPipeline(
                "SceneGridPipeline",
                GridPixelShader,
                { TextureFormat::R8G8B8A8_UNORM },
                TextureFormat::D32_FLOAT,
                rasterizerDesc,
                depthStencilDesc,
                DEFAULT_BLEND_DESC,
                WD_PRIMITIVE_TOPOLOGY_TYPE_LINE,
                inputElementDescs
            );

            ECS::World.observer<SceneGridComponent>().event(flecs::OnAdd).each([&](flecs::entity entity, SceneGridComponent&)
            {
                MarkGridDirty(entity);
            });

            ECS::World.observer<SceneGridComponent>().event(flecs::OnSet).each([&](flecs::entity entity, SceneGridComponent&)
            {
                MarkGridDirty(entity);
            });

            ECS::World.observer<SceneGridComponent>().event(flecs::OnRemove).each([&](flecs::entity entity, SceneGridComponent&)
            {
                ReleaseGridBuffer(entity);
            });

            ECS::World.observer<Transform, SceneGridComponent>().event(flecs::OnSet).each([&](flecs::entity entity, Transform&, SceneGridComponent&)
            {
                MarkGridDirty(entity);
            });

            ECS::World.observer<Transform>().event(flecs::OnRemove).each([&](flecs::entity entity, Transform&)
            {
                ReleaseGridBuffer(entity);
            });

            ECS::World.observer<Transform, MeshComponent>().event(flecs::OnAdd).each([&](flecs::entity, Transform&, MeshComponent&)
            {
                MarkAllGridDataDirty();
            });

            ECS::World.observer<Transform, MeshComponent>().event(flecs::OnSet).each([&](flecs::entity, Transform&, MeshComponent&)
            {
                MarkAllGridDataDirty();
            });

            ECS::World.observer<MeshComponent>().event(flecs::OnRemove).each([&](flecs::entity, MeshComponent&)
            {
                MarkAllGridDataDirty();
            });

            ECS::World.observer<Transform, Light>().event(flecs::OnAdd).each([&](flecs::entity, Transform&, Light&)
            {
                MarkAllGridDataDirty();
            });

            ECS::World.observer<Transform, Light>().event(flecs::OnSet).each([&](flecs::entity, Transform&, Light&)
            {
                MarkAllGridDataDirty();
            });

            ECS::World.observer<Light>().event(flecs::OnRemove).each([&](flecs::entity, Light&)
            {
                MarkAllGridDataDirty();
            });

            ECS::World.system("SceneGridSystem").kind<ECS::OnDraw>().each([&]
            {
                auto viewport = Renderer::GetCurrentViewport();
                ECS::Entity linkedCamera;
                if(!viewport->TryGetLinkedCamera(linkedCamera))
                {
                    return;
                }

                if(!linkedCamera.is_alive() || !linkedCamera.has<Camera>() || !linkedCamera.has<Transform>())
                {
                    return;
                }

                auto& camera = linkedCamera.get_mut<Camera>();
                auto gbuffer = viewport->GetGBuffer();
                SceneData.ViewProjection = camera.ViewProjectionMatrix;
                Renderer::UploadBuffer(SceneDataBuffer, &SceneData, sizeof(GridSceneData));

                gbuffer->Barrier(Depth, ALL_SHADER_RESOURCE, DEPTH_READ);
                Renderer::SetPipeline(GridPipeline);
                Renderer::BindRenderTargets(viewport->FrameBuffer->GetCurrentRenderTarget());
                Renderer::BindDepthStencil(gbuffer->GetRenderTarget(Depth));
                Renderer::SetVertexBuffers(GridVertexBuffer, 1);
                Renderer::SetIndexBuffer(GridIndexBuffer);

                auto gridQuery = ECS::World.query_builder<Transform, SceneGridComponent>().build();
                gridQuery.each([&](flecs::entity entity, Transform& transform, SceneGridComponent& grid)
                {
                    EnsureRenderDataEntry(entity);
                    auto& renderData = GridMeshes[entity];
                    if(AllGridsDirty || renderData.Dirty)
                    {
                        RebuildGridInstances(entity, transform, grid);
                        if(!GridMeshes.Contains(entity))
                        {
                            return;
                        }
                    }

                    if(renderData.InstanceCount == 0 || !renderData.InstanceBuffer)
                    {
                        return;
                    }

                    GridRootConstants rootConstants;
                    rootConstants.InstanceBufferID = renderData.InstanceBuffer->GetIndex(SRV_CBV);
                    rootConstants.SceneDataBufferID = SceneDataBuffer->GetIndex(SRV_CBV);
                    Renderer::PushConstants(&rootConstants, sizeof(GridRootConstants));
                    Renderer::DrawIndexedInstanced(GridIndexCount, renderData.InstanceCount, 0, 0, 0);
                });

                AllGridsDirty = false;
                gbuffer->Barrier(Depth, DEPTH_READ, ALL_SHADER_RESOURCE);
            });

            IsInitialized = true;
        }

        void Deinitialize() override
        {
            for(auto& pair : GridMeshes)
            {
                if(pair.value.InstanceBuffer)
                {
                    Renderer::Destroy(pair.value.InstanceBuffer);
                }
            }
            GridMeshes.Clear();

            if(GridVertexBuffer)
            {
                Renderer::Destroy(GridVertexBuffer);
                GridVertexBuffer = nullptr;
            }

            if(GridIndexBuffer)
            {
                Renderer::Destroy(GridIndexBuffer);
                GridIndexBuffer = nullptr;
            }

            if(SceneDataBuffer)
            {
                Renderer::Destroy(SceneDataBuffer);
                SceneDataBuffer = nullptr;
            }

            IsInitialized = false;
        }
    };
}
