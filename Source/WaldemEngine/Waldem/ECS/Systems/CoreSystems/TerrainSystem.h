#pragma once
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Terrain.h"
#include "Waldem/Renderer/Model/TerrainMesh.h"

namespace Waldem
{
    class WALDEM_API TerrainSystem : public ISystem
    {
    public:
        TerrainSystem() {}

        float SampleHeightBilinear(const uint8* data, int width, int height, float u, float v)
        {
            u = glm::clamp(u, 0.0f, 1.0f);
            v = glm::clamp(v, 0.0f, 1.0f);

            const float x = u * (width - 1);
            const float y = v * (height - 1);

            const int x0 = (int)floor(x);
            const int y0 = (int)floor(y);
            const int x1 = std::min(x0 + 1, width - 1);
            const int y1 = std::min(y0 + 1, height - 1);

            const float tx = x - x0;
            const float ty = y - y0;

            auto SampleTexel = [&](int px, int py)
            {
                const int pixelIndex = py * width + px;
                const int byteIndex = pixelIndex * 4;
                return data[byteIndex] / 255.0f;
            };

            const float h00 = SampleTexel(x0, y0);
            const float h10 = SampleTexel(x1, y0);
            const float h01 = SampleTexel(x0, y1);
            const float h11 = SampleTexel(x1, y1);

            const float hx0 = glm::mix(h00, h10, tx);
            const float hx1 = glm::mix(h01, h11, tx);
            return glm::mix(hx0, hx1, ty);
        }

        float SampleHeightSmoothed(const uint8* data, int width, int height, float u, float v)
        {
            const float du = 1.0f / (width - 1);
            const float dv = 1.0f / (height - 1);

            float sum = 0.0f;
            float weightSum = 0.0f;

            for (int offsetY = -1; offsetY <= 1; ++offsetY)
            {
                for (int offsetX = -1; offsetX <= 1; ++offsetX)
                {
                    const float weight =
                        (offsetX == 0 && offsetY == 0) ? 4.0f :
                        (offsetX == 0 || offsetY == 0) ? 2.0f : 1.0f;

                    sum += SampleHeightBilinear(data, width, height, u + offsetX * du, v + offsetY * dv) * weight;
                    weightSum += weight;
                }
            }

            return sum / weightSum;
        }

        void RecalculateTerrainNormals(StaticMesh* mesh, int resolution)
        {
            auto GetPosition = [&](int x, int y) -> Vector3
            {
                return mesh->VertexData[y * resolution + x].Position;
            };

            for (int y = 0; y < resolution; ++y)
            {
                for (int x = 0; x < resolution; ++x)
                {
                    const int leftX = std::max(x - 1, 0);
                    const int rightX = std::min(x + 1, resolution - 1);
                    const int downY = std::max(y - 1, 0);
                    const int upY = std::min(y + 1, resolution - 1);

                    const Vector3 left = GetPosition(leftX, y);
                    const Vector3 right = GetPosition(rightX, y);
                    const Vector3 down = GetPosition(x, downY);
                    const Vector3 up = GetPosition(x, upY);

                    const Vector3 dx = right - left;
                    const Vector3 dz = up - down;
                    const Vector3 normal = normalize(cross(dz, dx));

                    mesh->VertexData[y * resolution + x].Normal = Vector4(normal, 0.0f);
                }
            }
        }

        void ApplyHeightMap(ECS::Entity entity, Terrain& terrain, MeshComponent& meshComponent, StaticMesh* mesh)
        {
            const auto data = terrain.Heightmap.Texture->Desc.Data;
            const auto width = terrain.Heightmap.Texture->Desc.Width;
            const auto height = terrain.Heightmap.Texture->Desc.Height;

            for (int i = 0; i < terrain.Resolution; ++i) //rows
            {
                for (int j = 0; j < terrain.Resolution; ++j) //columns
                {
                    const int meshLinearIndex = i * terrain.Resolution + j;
                    const float u = j / (float)(terrain.Resolution - 1);
                    const float v = i / (float)(terrain.Resolution - 1);
                    const float sampledHeight = SampleHeightSmoothed(data, width, height, u, v);

                    mesh->VertexData[meshLinearIndex].Position.y = sampledHeight * terrain.Height;
                }
            }

            RecalculateTerrainNormals(mesh, terrain.Resolution);
             
            Renderer::RenderData.VertexBuffer.UpdateData(mesh->VertexData.GetData(), mesh->VertexData.GetSize(), meshComponent.DrawCommand.BaseVertexLocation * Renderer::RenderData.VertexBuffer.Stride);

            int globalDrawId;

            if(IdManager::GetId(entity, GlobalDrawIdType, globalDrawId))
            {
                Renderer::RenderData.TLAS.UpdateGeometry(globalDrawId, Renderer::RenderData.VertexBuffer.GetBuffer(), Renderer::RenderData.IndexBuffer.GetBuffer(), meshComponent.DrawCommand, mesh->VertexData.Num());
            }
        }
        
        void Initialize() override
        {
            Observer<Terrain>("OnTerrainAdded", flecs::OnAdd, [&](ECS::Entity entity, Terrain& terrain)
            {
                entity.set<MeshComponent>({MeshReference(new TerrainMesh(terrain.Resolution))});
            });
            
            Observer<Terrain>("OnTerrainChanged", flecs::OnSet, [&](ECS::Entity entity, Terrain& terrain)
            {
                auto& meshComponent = entity.get_mut<MeshComponent>();
                
                auto terrainMesh = meshComponent.MeshRef.Mesh;
                
                bool needToApplyHeightMap = false;
                
                if(terrain.Resolution != (int)terrain.InitializedResolution)
                {
                    terrainMesh = new TerrainMesh(terrain.Resolution);
                    terrain.InitializedResolution = terrain.Resolution;
                    needToApplyHeightMap = true;
                }

                if(terrain.Height != terrain.InitializedHeight)
                {
                    needToApplyHeightMap = true;
                    terrain.InitializedHeight = terrain.Height;
                }
                
                if(terrain.InitializedReference != terrain.Heightmap.Reference)
                {
                    terrain.Heightmap.LoadAsset();
                
                    terrain.InitializedReference = terrain.Heightmap.Reference; 
                    needToApplyHeightMap = true;
                }

                if(needToApplyHeightMap)
                {
                    if(terrain.Heightmap.IsValid())
                    {
                        ApplyHeightMap(entity, terrain, meshComponent, terrainMesh);
                    }
                }
                
                meshComponent.MeshRef.Mesh = terrainMesh;
                entity.modified<MeshComponent>();
            });
        }
    };
}
