#version 330 core

layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Color;
uniform mat4 world;
uniform mat4 viewProjection;
out vec3 OutPosition;
out vec4 OutColor;

void main()
{
    OutColor = Color;
    OutPosition = Position;
    gl_Position = viewProjection * world * vec4(Position, 1.0);
}