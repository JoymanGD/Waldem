#pragma once
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/ECS/Component.h"

namespace Waldem
{
    struct WALDEM_API MeshComponent : IComponent<MeshComponent>
    {
        CMesh* Mesh = nullptr;
        
        MeshComponent() = default;
        MeshComponent(CMesh* mesh) : Mesh(mesh) {}
        
        void Serialize(WDataBuffer& outData) override {}
        void Deserialize(WDataBuffer& inData) override {}
    };
}
