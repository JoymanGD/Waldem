#version 420

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InUV;
uniform mat4 world;
uniform mat4 viewProjection;
out vec3 Position;
out vec3 Normal;
out vec2 UV;

void main()
{
    Position = InPosition;
    Position.x *= -1;
    Normal = InNormal;
    UV = InUV;
    gl_Position = viewProjection * world * vec4(Position, 1.0);
}