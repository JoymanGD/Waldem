#pragma once
#include "Waldem/Editor/AssetReference/TextureReference.h"

namespace Waldem
{
    COMPONENT()
    struct Sprite
    {
        FIELD()
        Vector4 Color = Vector4(1,1,1,1); // tint
        FIELD()
        TextureReference TextureRef;
        FIELD()
        bool AlphaCut;
        
        DrawIndexedCommand DrawCommand;
        
        Sprite() {}
    };
}
#include "Sprite.generated.h"
