#pragma once
#include "Texture.h"

class Material
{
public:
    Material() = default;
    Material(Texture* diffuseTexture)
    {
        DiffuseTexture = diffuseTexture;
    }
    
    void SetTexturesParams(uint32_t shaderProgramID)
    {
        DiffuseTexture->SetParam(shaderProgramID);
    }
    
    void Bind()
    {
        DiffuseTexture->Bind();
    }

    void Unbind()
    {
        DiffuseTexture->Unbind();
    }

    void Destroy()
    {
        DiffuseTexture->Destroy();
    }
private:
    Texture* DiffuseTexture;
};
