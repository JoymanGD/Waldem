#version 420

#define MAX_LIGHTS 25

struct Light
{
    vec3 Position;
    uint Type;
    vec3 Direciton;
    float Intensity;
    vec3 Color;
    float Range;
};

layout(location = 0) out vec4 Color;
in vec3 Position;
in vec3 Normal;
in vec2 UV;
uniform sampler2D DiffuseTexture;
uniform uint NumLights;
layout(std140, binding = 0) uniform LightsBlock
{
    Light Lights[MAX_LIGHTS];
};

void main()
{
    vec4 textureColor = vec4(texture(DiffuseTexture, UV));
    
    if(textureColor.a < 0.33f)
    {
        discard;
    }
    
    vec4 finalColor = textureColor;
    
    for(int i = 0; i < NumLights; i++)
    {
        Light light = Lights[i];
        
        if(light.Type == 0)
        {
            finalColor *= vec4(light.Color, 1.0f);
        }
    }
        
    Color = finalColor;
}