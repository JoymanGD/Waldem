#version 330 core

layout(location = 0) out vec4 Color;
in vec3 OutPosition;
in vec3 OutNormal;
in vec4 OutColor;
in vec2 OutUV;
uniform sampler2D DiffuseTexture;

void main()
{
    //Color = vec4(texture(DiffuseTexture, OutUV));
    Color = vec4(abs(OutNormal), 1.0f);
}