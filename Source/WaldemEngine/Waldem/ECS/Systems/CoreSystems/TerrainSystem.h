#pragma once
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Terrain.h"

namespace Waldem
{
    class WALDEM_API TerrainSystem : public ICoreSystem
    {
    public:
        TerrainSystem() {}
        
        void Initialize() override
        {
            ECS::World.observer<Terrain>().event(flecs::OnAdd).each([&](flecs::entity entity, Terrain& terrain)
            {
                entity.set<MeshComponent>({MeshReference(new TerrainMesh(terrain.Resolution))});
            });
            
            ECS::World.observer<Terrain>().event(flecs::OnSet).each([&](flecs::entity entity, Terrain& terrain)
            {
                if(terrain.Resolution != terrain.InitializedResolution)
                {
                    auto& meshComponent = entity.get_mut<MeshComponent>();
                    meshComponent.MeshRef.Mesh = new TerrainMesh(terrain.Resolution);
                    entity.modified<MeshComponent>();
                    terrain.InitializedResolution = terrain.Resolution;
                }
            });
        }
    };
}
