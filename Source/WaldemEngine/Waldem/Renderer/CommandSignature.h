#pragma once
#include "Interfaces/IGraphicObject.h"

namespace Waldem
{
    class WALDEM_API CommandSignature : public IGraphicObject
    {
    public:
        CommandSignature() = default;
        virtual ~CommandSignature() = default;
    };
}
