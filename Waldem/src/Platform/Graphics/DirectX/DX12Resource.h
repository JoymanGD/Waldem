#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Resource.h"

namespace Waldem
{
    struct ResourceData
    {
        ID3D12Resource* DX12Resource;
        Resource Desc;
    };
}
