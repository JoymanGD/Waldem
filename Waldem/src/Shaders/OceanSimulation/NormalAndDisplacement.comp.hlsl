#include "OceanResources.hlsl"

Texture2D Dx : register(t0);
Texture2D Dy : register(t1);
Texture2D Dz : register(t2);
RWTexture2D<float4> Normal : register(u0);
RWTexture2D<float4> Displacement : register(u1);

[numthreads(16, 16, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    Displacement[tid] = float4(Dx[tid].r, Dy[tid].r, Dz[tid].r, 1.0);

    float z0 = Dy[tid + uint2(-1,-1)].r;
    float z1 = Dy[tid + uint2(0,-1)].r;
    float z2 = Dy[tid + uint2(1,-1)].r;
    float z3 = Dy[tid + uint2(-1,0)].r;
    float z4 = Dy[tid + uint2(1,0)].r;
    float z5 = Dy[tid + uint2(-1,1)].r;
    float z6 = Dy[tid + uint2(0,1)].r;
    float z7 = Dy[tid + uint2(1,1)].r;
	 
    float3 normal;
	
    // Sobel Filter
    normal.z = 1.0/NormalStrength;
    normal.x = z0 + 2*z3 + z5 - z2 - 2*z4 - z7;
    normal.y = z0 + 2*z1 + z2 -z5 - 2*z6 - z7;
    normal = normalize(normal);

    float3 encodedNormal = normal * 0.5 + 0.5;

    Normal[tid] = float4(encodedNormal, 1.0);
}