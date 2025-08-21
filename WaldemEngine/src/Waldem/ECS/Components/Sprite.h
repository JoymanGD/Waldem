#pragma once
#include "Waldem/Editor/AssetReference/TextureReference.h"

namespace Waldem
{
    struct Sprite
    {
        COMPONENT(Sprite)
            FIELD(AssetReference, Texture)
            FIELD(Vector4, Color)
            FIELD(Vector4, Rect)
        END_COMPONENT()
        
        TextureReference TextureRef;
        Vector4 Color = Vector4(1,1,1,1); // tint
        Vector4 Rect = Vector4(0,0,1,1); // UV region in atlas
    };
}
