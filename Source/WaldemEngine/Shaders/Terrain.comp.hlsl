#include "TerrainCommon.hlsl"

SamplerState myStaticSampler : register(s1);

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    StructuredBuffer<TerrainSceneData> sceneDataBuffer = ResourceDescriptorHeap[SceneDataBufferId];
    TerrainSceneData terrainData = sceneDataBuffer[0];
    
    uint resolution = terrainData.Resolution;

    if (tid.x >= resolution || tid.y >= resolution)
        return;

    if(terrainData.HeightMapIndex > 0)
    {
        RWStructuredBuffer<TerrainVertex> vertexBuffer = ResourceDescriptorHeap[terrainData.VertexBufferIndex];
        RWStructuredBuffer<uint> indexBuffer = ResourceDescriptorHeap[terrainData.IndexBufferIndex];
        RWStructuredBuffer<uint> physXSamplesBuffer = ResourceDescriptorHeap[terrainData.PhysXSamplesBufferIndex];
        
        int vertexIndex = tid.y * resolution + tid.x;
        int sampleIndex = tid.x * resolution + tid.y;
        
        //displacement
        if(terrainData.HeightMapIndex > 0)
        {
            float2 uv = float2(tid)/float(resolution-1);
            
            Texture2D<float> heightMap = ResourceDescriptorHeap[NonUniformResourceIndex(terrainData.HeightMapIndex)];
            float height01 = heightMap.SampleLevel(myStaticSampler, uv, 0.0f).r;
            float3 localPos = float3(uv.x * DEFAULT_TERRAIN_SIZE, height01 * terrainData.Height, uv.y * DEFAULT_TERRAIN_SIZE);

            //normals
            uint width, height;
            heightMap.GetDimensions(width, height);
            float2 texelSize = 1.0 / float2(width, height);
            float hL = heightMap.SampleLevel(myStaticSampler, uv - float2(texelSize.x, 0.0), 0.0).r * terrainData.Height;
            float hR = heightMap.SampleLevel(myStaticSampler, uv + float2(texelSize.x, 0.0), 0.0).r * terrainData.Height;
            float hD = heightMap.SampleLevel(myStaticSampler, uv - float2(0.0, texelSize.y), 0.0).r * terrainData.Height;
            float hU = heightMap.SampleLevel(myStaticSampler, uv + float2(0.0, texelSize.y), 0.0).r * terrainData.Height;
            float stepX = DEFAULT_TERRAIN_SIZE  * texelSize.x;
            float stepZ = DEFAULT_TERRAIN_SIZE  * texelSize.y;
            float3 dx = float3(2.0 * stepX, hR - hL, 0.0);
            float3 dz = float3(0.0, hU - hD, 2.0 * stepZ);
            
            float3 normal = normalize(cross(dz, dx));

            //PhysX sample
            // int int16Height = height01 * 32767;
            int int16Height = (int)round(height01 * 32767.0f);
            uint uint8Material1 = 0;
            uint uint8Material2 = 0;
            // physXSample = ((uint)(int16Height & 0xFFFF) << 16) | ((uint8Material1 & 0xFF) << 8) | (uint8Material2 & 0xFF);
            uint physXSample = (uint)(int16Height & 0xFFFF) | (uint8Material1 & 0xFF) << 16 | (uint8Material2 & 0xFF) << 24;
        
            vertexBuffer[vertexIndex].Position = localPos;
            vertexBuffer[vertexIndex].Normal = normal;
            vertexBuffer[vertexIndex].UV = uv;
            physXSamplesBuffer[sampleIndex] = physXSample;
        }

        //indices
        if(tid.x < resolution - 1 && tid.y < resolution - 1)
        {
            uint topLeft = vertexIndex;
            uint topRight = vertexIndex + 1;
            uint bottomLeft = vertexIndex + resolution;
            uint bottomRight = bottomLeft + 1;
            
            uint quadIndex = tid.y * (resolution - 1) + tid.x;
            uint firstIndex = quadIndex * 6;
            
            indexBuffer[firstIndex + 0] = topLeft;
            indexBuffer[firstIndex + 1] = bottomLeft;
            indexBuffer[firstIndex + 2] = topRight;
                        
            indexBuffer[firstIndex + 3] = topRight;
            indexBuffer[firstIndex + 4] = bottomLeft;
            indexBuffer[firstIndex + 5] = bottomRight;
        }
    }
}
