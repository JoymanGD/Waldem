#pragma once

namespace Waldem
{    
    class Texture2D
    {
    public:
        virtual ~Texture2D() {}
        virtual std::string GetName() = 0;
        virtual void* GetPlatformResource() = 0;
    };
}