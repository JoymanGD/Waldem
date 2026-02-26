#pragma once
#include <string>
#include "GraphicTypes.h"
#include "Interfaces/IGraphicObject.h"

namespace Waldem
{    
    class WALDEM_API Pipeline : public IGraphicObject
    {
    public:
        Pipeline(const WString& name) : Name(name) {}
        virtual ~Pipeline() = default;
        PipelineType CurrentPipelineType; 
    protected:
        WString Name;
    };
}
