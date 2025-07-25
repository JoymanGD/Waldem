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
            ResourceManager::SerializeAsset(outData, Mesh);
        }
        void Deserialize(WDataBuffer& inData) override
        {
            ResourceManager::DeserializeAsset(inData, Mesh);
        }
    };
}
