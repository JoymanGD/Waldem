#pragma once
#include "ComponentBase.h"

namespace Waldem
{
    struct WALDEM_API ParticleSystemComponent
    {
        COMPONENT(ParticleSystemComponent)
            FIELD(Vector4, Color)
            FIELD(Vector3, Size)
            FIELD(uint, ParticlesAmount)
            FIELD(Vector3, Acceleration)
            FIELD(float, Lifetime)
        END_COMPONENT()
        
        Vector4 Color = Vector4(1, 1, 1, 1);
        Vector3 Size = Vector3(1.f, 1.f, 1.f);
        uint ParticlesAmount = 1;
        Vector3 Acceleration = Vector3(0, -9.81f, 0);
        float Lifetime = 5.0f;
    };
}
