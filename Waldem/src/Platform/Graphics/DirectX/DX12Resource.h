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
        D3D12_RESOURCE_STATES CurrentState = D3D12_RESOURCE_STATE_COMMON;

        ResourceData(Resource desc, uint32_t rootParameterIndex, D3D12_RESOURCE_STATES initialState)
            : Desc(desc), RootParameterIndex(rootParameterIndex), CurrentState(initialState)
        {
        }

        ResourceData(ID3D12Resource* uploadResource, ID3D12Resource* defaultResource, ID3D12Resource* readbackResource, Resource desc, uint32_t rootParameterIndex, D3D12_RESOURCE_STATES initialState)
            : Desc(desc), DX12UploadResource(uploadResource), DX12DefaultResource(defaultResource), DX12ReadbackResource(readbackResource), RootParameterIndex(rootParameterIndex), CurrentState(initialState)
        {
        }
    };

    struct ResourcePair
    {
        String Name;
        ResourceData* ResourceData = nullptr;
    };
}
