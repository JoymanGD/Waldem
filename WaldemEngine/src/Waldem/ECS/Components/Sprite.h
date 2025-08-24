#pragma once
#include "Waldem/Editor/AssetReference/TextureReference.h"

namespace Waldem
{
    struct Sprite
    {
        COMPONENT(Sprite)
            FIELD(Vector4, Color)
            FIELD(AssetReference, Texture)
        END_COMPONENT()
        
        Vector4 Color = Vector4(1,1,1,1); // tint
        TextureReference TextureRef;
        Vector4 Rect = Vector4(0,0,1,1); // UV region in atlas
    };
}
