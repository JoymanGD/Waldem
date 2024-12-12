#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Resource.h"

namespace Waldem
{
    struct ResourceData
    {
        ID3D12Resource* DX12UploadResource = nullptr;
        ID3D12Resource* DX12DefaultResource = nullptr;
        Resource Desc;
    };
}
