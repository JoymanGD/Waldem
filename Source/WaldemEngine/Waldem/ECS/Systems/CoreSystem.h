#pragma once

namespace Waldem
{
    class WALDEM_API ICoreSystem
    {
    protected:
        bool IsInitialized = false;
        bool IsActive = false;
    public:
        ICoreSystem() {}
        virtual void Initialize() {}
        virtual void Deinitialize() {}
        virtual void OnResize(Vector2 size) {}
        virtual void SetActive(bool active)
        {
            IsActive = active;
        }
    };
}
