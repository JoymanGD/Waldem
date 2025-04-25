#pragma once
#include "DX12CommandList.h"
#include <d3d12.h>
#include "Waldem/Renderer/Sampler.h"

namespace Waldem
{
    class WALDEM_API DX12Sampler : public Sampler
    {
    public:
        DX12Sampler(WD_SAMPLER_FILTER filter, WD_TEXTURE_ADDRESS_MODE addressU, WD_TEXTURE_ADDRESS_MODE addressV, WD_TEXTURE_ADDRESS_MODE addressW, float mipLODBias, uint32_t maxAnisotropy, WD_COMPARISON_FUNC comparisonFunc, float minLOD, float maxLOD)
            : Sampler(filter, addressU, addressV, addressW, mipLODBias, maxAnisotropy, comparisonFunc, minLOD, maxLOD) {}
    };
}