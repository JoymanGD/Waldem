#pragma once
#include "Renderer.h"
#include "Waldem/Renderer/RenderTarget.h"

namespace Waldem
{
    enum class LightType
    {
        Directional = 0,
        Point = 1,
    };
    
    struct LightData
    {
        Vector3 Color;
        float Intensity;
        LightType Type;
        float Range;
        Matrix4 Projection;
    };
    
    struct LightShaderData
    {
        LightShaderData(LightData lightData, Transform transform)
        {
            Color = lightData.Color;
            Intensity = lightData.Intensity;
            Type = lightData.Type;
            Range = lightData.Range;
            World = transform.Matrix;
            View = transform.Inverse();
            Projection = lightData.Projection;
        }
        
        Vector3 Color;
        float Intensity;
        Vector2 Padding1;
        LightType Type;
        float Range;
        Matrix4 World;
        Matrix4 View;
        Matrix4 Projection;
    };
    
    struct Light
    {
        Light(Vector3 Color, float Intensity, LightType Type, float Range)
        {
            Data.Color = Color;
            Data.Intensity = Intensity;
            Data.Type = Type;
            Data.Range = Range;
            Data.Projection = glm::orthoZO(-40.0f, 40.0f, -40.0f, 40.0f, -200.f, 200.0f);
            
            Shadowmap = Renderer::CreateRenderTarget("ShadowmapRT", 4096, 4096, TextureFormat::D32_FLOAT);
        }
        
        LightData Data;
        RenderTarget* Shadowmap;
    };
}