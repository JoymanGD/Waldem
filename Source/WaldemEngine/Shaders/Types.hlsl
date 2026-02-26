struct Vertex
{
    float4 Position;
    float4 Color;
    float4 Normal;
    float4 Tangent;
    float4 Bitangent;
    float2 UV;
};

struct DrawCommand
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
};