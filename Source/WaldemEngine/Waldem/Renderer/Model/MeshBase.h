#pragma once
#include "Material.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Editor/AssetReference/MaterialReference.h"
#include "Waldem/Renderer/AABB.h"
#include "Waldem/Renderer/Buffer.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    struct Vertex
    {
        Vector4 Position;
        Vector4 Color;
        Vector4 Normal;
        Vector4 Tangent;
        Vector4 Bitangent;
        Vector2 UV;

        Vertex(Vector4 position, Vector4 color, Vector4 normal, Vector4 tangent, Vector4 bitangent, Vector2 uv) : Position(position), Color(color), Normal(normal), Tangent(tangent), Bitangent(bitangent), UV(uv) {}
        Vertex() {}
    };
    
    class WALDEM_API MeshBase : public Asset
    {
    public:
        MeshBase() {}
        MeshBase(WString name) : Asset(name, AssetType::Mesh) {}
        MeshBase(WArray<Vertex> vertexData, WArray<uint> indexData, Path materialPath, AABB bBox, WString name = "", Matrix4 objectMatrix = glm::identity<Matrix4>());
        
        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;
        
        Buffer* VertexBuffer = nullptr;
        Buffer* IndexBuffer = nullptr;
        Path MaterialPath = "Content/Materials/Default.mat";
        WArray<Vector3> Positions;

        WArray<Vertex> VertexData;
        WArray<uint> IndexData;
        AABB BBox;
        Matrix4 ObjectMatrix;
    };
}