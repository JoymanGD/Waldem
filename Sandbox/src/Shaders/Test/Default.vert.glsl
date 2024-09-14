#version 330 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec3 UV;
uniform mat4 world;
uniform mat4 viewProjection;
out vec3 OutPosition;
out vec3 OutNormal;
out vec4 OutColor;
out vec2 OutUV;

void main()
{
    OutPosition = Position;
    OutNormal = Normal;
    OutColor = Color;
    OutUV = UV.xy;
    gl_Position = viewProjection * world * vec4(OutPosition, 1.0);
}