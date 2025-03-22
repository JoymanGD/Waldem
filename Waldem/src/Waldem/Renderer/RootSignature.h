#pragma once
#include "Interfaces/IGraphicObject.h"

namespace Waldem
{
    class WALDEM_API RootSignature : public IGraphicObject
    {
    public:
        RootSignature() {}
        virtual void UpdateResourceData(String name, void* data) = 0;
        virtual void ReadbackResourceData(String name, void* destinationData) = 0;
        virtual void ClearResource(String name) = 0;
        virtual ~RootSignature() = default;
        PipelineType CurrentPipelineType; 
    };
}
