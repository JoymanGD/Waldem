#pragma once
#include "ComponentBase.h"

namespace Waldem
{
    enum class EmitterShape
    {
        Circle = 0,
        Sphere = 1
    };
    
    COMPONENT()
    struct WALDEM_API ParticleSystemComponent
    {
        FIELD()
        uint ParticlesAmount = 100;
        FIELD(Color)
        Vector4 Color = Vector4(1, 1, 1, 1);
        FIELD()
        Vector2 SizeMinMax = Vector2(1.f, 1.f);
        FIELD()
        Vector2 SpeedMinMax = Vector2(1, 1);
        FIELD()
        Vector2 LifetimeMinMax = Vector2(5.0f, 5.0f);
        FIELD()
        EmitterShape Shape = EmitterShape::Circle;
        FIELD()
        float Radius = 1;
        
        ParticleSystemComponent() {}
    };
}
#include "ParticleSystemComponent.generated.h"
