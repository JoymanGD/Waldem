#pragma once
#include <string>

#include "GraphicTypes.h"
#include "Interfaces/IGraphicObject.h"

namespace Waldem
{
    class WALDEM_API Pipeline : public IGraphicObject
    {
    public:
        Pipeline(const String& name) : Name(name) {}
        virtual ~Pipeline() = default; 
    protected:
        String Name;
    };
}
