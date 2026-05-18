#pragma once
#include "ComponentBase.h"
#include "Waldem/Editor/AssetReference/AnimationClipReference.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API AnimatorComponent
    {
        FIELD(Type=AnimationClipReference)
        AnimationClipReference ClipRef;

        FIELD()
        float PlaybackSpeed = 1.0f;

        FIELD()
        int Loop = 1;

        FIELD()
        bool PlayOnStart = false;

        int IsPlaying = 0;

        float CurrentTimeTicks = 0.0f;
        bool Started = false;

        AnimatorComponent() = default;

        void Play()
        {
            IsPlaying = 1;
        }

        void Stop()
        {
            IsPlaying = 0;
        }
    };
}
#include "AnimatorComponent.generated.h"
