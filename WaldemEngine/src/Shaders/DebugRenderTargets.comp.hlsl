RWTexture2D<float4> RenderTarget : register(u0);
Texture2D DebugRT_1 : register(t0);
Texture2D DebugRT_2 : register(t1);
Texture2D DebugRT_3 : register(t2);
Texture2D DebugRT_4 : register(t3);
Texture2D DebugRT_5 : register(t4);
Texture2D DebugRT_6 : register(t5);
Texture2D DebugRT_7 : register(t6);
Texture2D DebugRT_8 : register(t7);
Texture2D DebugRT_9 : register(t8);

cbuffer MyConstantBuffer : register(b0)
{
    float2 TargetResolution;
    float2 DebugResolution;
    uint DebugRTIndex;
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
    float2 UV = (float2)tid / TargetResolution;
    float2 debugPixelPos = UV * DebugResolution;

    uint2 debugRTStartPos = debugPixelPos;

    if(DebugRTIndex == 1)
    {
        RenderTarget[debugRTStartPos] = DebugRT_1.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 2)
    {
        RenderTarget[debugRTStartPos] = DebugRT_2.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 3)
    {
        RenderTarget[debugRTStartPos] = DebugRT_3.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 4)
    {
        RenderTarget[debugRTStartPos] = DebugRT_4.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 5)
    {
        RenderTarget[debugRTStartPos] = DebugRT_5.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 6)
    {
        RenderTarget[debugRTStartPos] = DebugRT_6.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 7)
    {
        RenderTarget[debugRTStartPos] = DebugRT_7.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 8)
    {
        RenderTarget[debugRTStartPos] = DebugRT_8.Load(uint3(debugPixelPos, 0));
    }
    else if(DebugRTIndex == 9)
    {
        RenderTarget[debugRTStartPos] = DebugRT_9.Load(uint3(debugPixelPos, 0));
    }
}
