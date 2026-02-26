#pragma once
#include "ComponentBase.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API ParticleSystemComponent
    {        
        FIELD()
        Vector4 Color = Vector4(1, 1, 1, 1);
        FIELD()
        Vector3 Size = Vector3(1.f, 1.f, 1.f);
        FIELD()
        uint ParticlesAmount = 1;
        FIELD()
        Vector3 Acceleration = Vector3(0, -9.81f, 0);
        FIELD()
        float Lifetime = 5.0f;
        
        uint BufferId = -1;
        
        ParticleSystemComponent() {}
    };
}
#include "ParticleSystemComponent.generated.h"
