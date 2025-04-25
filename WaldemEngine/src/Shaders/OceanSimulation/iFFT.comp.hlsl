RWTexture2D<float4> Displacement : register(u0);
Texture2D PingPong0 : register(t0);
Texture2D PingPong1 : register(t1);

cbuffer MyConstantBuffer : register(b0)
{
    uint N;
    uint pingpong;
}

[numthreads(16, 16, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float perms[] = { 1.0f, -1.0f };
    int index = int(fmod(int(tid.x + tid.y), 2));
    float perm = perms[index];

    if(pingpong == 0)
    {
        float h = PingPong0.Load(int3(tid, 0)).r;
        float d = perm*(h/float(N*N));
        Displacement[tid] = float4(d, d, d, 1); 
    }
    else if(pingpong == 1)
    {
        float h = PingPong1.Load(int3(tid, 0)).r;
        float d = perm*(h/float(N*N));
        Displacement[tid] = float4(d, d, d, 1); 
    }
}