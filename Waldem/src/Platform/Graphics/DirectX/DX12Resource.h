#pragma once
#include <d3d12.h>
#include "Waldem/Renderer/Resource.h"

namespace Waldem
{
    struct ResourceData
    {
        ID3D12Resource* DX12UploadResource = nullptr;
        ID3D12Resource* DX12DefaultResource = nullptr;
        ID3D12Resource* DX12ReadbackResource = nullptr;
        Resource Desc;
        uint32_t RootParameterIndex = 0;

        ResourceData(Resource desc, uint32_t rootParameterIndex)
            : Desc(desc), RootParameterIndex(rootParameterIndex)
        {
        }

        ResourceData(ID3D12Resource* uploadResource, ID3D12Resource* defaultResource, ID3D12Resource* readbackResource, Resource desc, uint32_t rootParameterIndex)
            : Desc(desc), DX12UploadResource(uploadResource), DX12DefaultResource(defaultResource), DX12ReadbackResource(readbackResource), RootParameterIndex(rootParameterIndex)
        {
        }
    };

    struct ResourcePair
    {
        String Name;
        ResourceData* ResourceData = nullptr;
    };
}
