#pragma once

namespace Waldem
{
    class WALDEM_API IGraphicObject
    {
    public:
        virtual void* GetNativeObject() const = 0;
        virtual void Destroy() = 0;
    };
}
