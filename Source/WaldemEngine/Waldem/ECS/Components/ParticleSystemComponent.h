#pragma once
#include "ComponentBase.h"
#include "Waldem/Types/WColor.h"
#include "Waldem/Types/WGradient.h"

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
        WColor Color = WColor(1, 1, 1, 1);
        FIELD()
        WGradient ColorOverLifetime;
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
        FIELD()
        bool WorldSpace = false;

        uint8 _WorldSpacePadding[3] = {};
        
        ParticleSystemComponent() {}
    };
}
#include "ParticleSystemComponent.generated.h"
