struct MaterialAttribute
{
    int DiffuseTextureIndex;
    int NormalTextureIndex;
    int ORMTextureIndex;
    int ClearCoatTextureIndex;

    float4 Albedo;
    float Metallic;
    float Roughness;
    int AlphaCut;
};
