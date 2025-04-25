#pragma once
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Resources/ResourceManager.h"
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API MeshComponent : IComponent<MeshComponent>
    {
        CMesh* Mesh = nullptr;
        
        MeshComponent() = default;
        MeshComponent(CMesh* mesh) : Mesh(mesh) {}
        
        void Serialize(WDataBuffer& outData) override
        {
            uint64 hash = ResourceManager::ExportAsset(Mesh);
            outData << hash;
        }
        void Deserialize(WDataBuffer& inData) override
        {
            uint64 hash;
            inData >> hash;
            Mesh = ResourceManager::ImportAsset<CMesh>(hash);
        }
    };
}
