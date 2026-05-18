#pragma once
#include "Material.h"
#include "MeshBase.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Editor/AssetReference/MaterialReference.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{    
    class WALDEM_API StaticMesh : public MeshBase
    {
    public:
        StaticMesh() {}
        StaticMesh(WString name) : MeshBase(name) {}
    };
}