#pragma once
#include "Interfaces/IGraphicObject.h"

namespace Waldem
{
    class WALDEM_API RootSignature : public IGraphicObject
    {
    public:
        RootSignature() {}
        virtual void UpdateResourceData(WString name, void* data) = 0;
        virtual void ReadbackResourceData(WString name, void* destinationData) = 0;
        virtual void ClearResource(WString name) = 0;
        virtual ~RootSignature() = default;
        PipelineType CurrentPipelineType; 
    };
}
