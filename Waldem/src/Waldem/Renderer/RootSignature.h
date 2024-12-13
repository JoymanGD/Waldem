#pragma once
#include "Resource.h"
#include "Interfaces/IGraphicObject.h"

namespace Waldem
{
    class WALDEM_API RootSignature : public IGraphicObject
    {
    public:
        RootSignature() {}
        virtual void UpdateResourceData(String name, void* data) = 0;
        virtual ~RootSignature() = default;
    };
}
