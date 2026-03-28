#pragma once
#include "ComponentBase.h"
#include "Waldem/Editor/AssetReference/ScriptReference.h"

namespace Waldem
{
    COMPONENT()
    struct WALDEM_API ScriptComponent
    {
        FIELD()
        ScriptReference Script;

        FIELD()
        bool Enabled = true;

        FIELD(Hidden)
        WString FieldOverrides;
    };
}

#include "ScriptComponent.generated.h"
