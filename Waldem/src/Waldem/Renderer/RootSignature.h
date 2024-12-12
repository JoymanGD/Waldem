#pragma once
#include "Interfaces/IGraphicObject.h"

namespace Waldem
{
    class WALDEM_API RootSignature : public IGraphicObject
    {
    public:
        RootSignature();
        virtual ~RootSignature() = default;
    };
}
