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
        Vector3 Position;
        Vector4 Color;
        Vector3 Normal;
        Vector3 Tangent;
        Vector3 Bitangent;
        Vector2 UV;

        Vertex(Vector3 position, Vector4 color, Vector3 normal, Vector3 tangent, Vector3 bitangent, Vector2 uv) : Position(position), Color(color), Normal(normal), Tangent(tangent), Bitangent(bitangent), UV(uv) {}
        Vertex() {}
    };
    
    class WALDEM_API CMesh : public Asset
    {
    public:
        CMesh() {}
        CMesh(WString name) : Asset(name, AssetType::Mesh) {}
        CMesh(WArray<Vertex> vertexData, WArray<uint> indexData, MaterialReference materialRef, AABB bBox, WString name = "", Matrix4 objectMatrix = glm::identity<Matrix4>());
        
        void Serialize(WDataBuffer& outData) override;
        void Deserialize(WDataBuffer& inData) override;

        Buffer* VertexBuffer = nullptr;
        Buffer* IndexBuffer = nullptr;
        MaterialReference MaterialRef = {};
        WArray<Vector3> Positions;
        WArray<Vertex> VertexData;
        WArray<uint> IndexData;
        AABB BBox;
        Matrix4 ObjectMatrix;
    };
}