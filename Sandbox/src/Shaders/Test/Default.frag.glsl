#version 330 core

layout(location = 0) out vec4 Color;
in vec3 OutPosition;
in vec4 OutColor;

void main()
{
    Color = OutColor;
}