#pragma once
#include "../Texture.h"
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class Material
    {
    public:
        Material() = default;
        Material(Texture2D* diffuseTexture)
        {
            DiffuseTexture = diffuseTexture;
        }
        
        void BindTexturesToShader(PixelShader* shader)
        {
            shader->SetParam(ShaderParamType::TEXTURE2D, DiffuseTexture->GetName().c_str(), DiffuseTexture->GetSlot());
        }
        
        void Bind()
        {
            DiffuseTexture->Bind();
        }

        void Unbind()
        {
            DiffuseTexture->Unbind();
        }
    private:
        Texture2D* DiffuseTexture;
    };
}
