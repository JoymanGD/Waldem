#pragma once

namespace Waldem
{
    class WALDEM_API ICoreSystem
    {
    protected:
        bool IsInitialized = false;
    public:
        ICoreSystem() {}
        virtual void Initialize() {}
        virtual void Deinitialize() {}
        virtual void OnResize(Vector2 size) {}
    };
}
