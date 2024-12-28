#pragma once
#include <comdef.h>
#include "D3DX12.h"

namespace Waldem
{
    class WALDEM_API DX12Helper
    {
    public:
        static String MBFromW(LPCWSTR pwsz, UINT cp)
        {
            int cch = WideCharToMultiByte(cp, 0, pwsz, -1, 0, 0, NULL, NULL);

            char* psz = new char[cch];

            WideCharToMultiByte(cp, 0, pwsz, -1, psz, cch, NULL, NULL);

            String st(psz);
            delete[] psz;

            return st;
        }

        static LPCWSTR WFromMB(const String& str)
        {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
            std::wstring wstrTo(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
            return wstrTo.c_str();
        }

        static std::wstring StringToLPCWSTR(const String& str)
        {
            return std::wstring(str.begin(), str.end());
        }
        
        static void PrintHResultError(HRESULT hr, bool throwException = true)
        {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            auto str = MBFromW(errMsg, 0);
            WD_CORE_ERROR("DX12 Error: {0}", str);

            if(throwException)
            {
                throw std::runtime_error(str);
            }
        }

        static void PrintDeviceRemovedReason(ID3D12Device* device)
        {
            HRESULT reason = device->GetDeviceRemovedReason();

            String errorStr = "";

            switch (reason)
            {
            case DXGI_ERROR_DEVICE_HUNG:
                errorStr = "DXGI_ERROR_DEVICE_HUNG. The GPU is in an unusable state.";
                break;
            case DXGI_ERROR_DEVICE_REMOVED:
                errorStr = "DXGI_ERROR_DEVICE_REMOVED. The GPU was physically removed or driver crashed.";
                break;
            case DXGI_ERROR_DEVICE_RESET:
                errorStr = "DXGI_ERROR_DEVICE_RESET. The GPU was reset due to a timeout.";
                break;
            case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
                errorStr = "DXGI_ERROR_DRIVER_INTERNAL_ERROR. Internal driver error.";
                break;
            case DXGI_ERROR_INVALID_CALL:
                errorStr = "DXGI_ERROR_INVALID_CALL. The application made an invalid call.";
                break;
            case S_OK:
                errorStr = "Device is functioning normally.";
                break;
            default:
                errorStr = "Unknown error.";
                break;
            }

            WD_CORE_ERROR("DX12 device removed: {0}", errorStr);
        }

        static D3D12_DESCRIPTOR_RANGE_TYPE ResourceTypeToRangeType(ResourceType resourceType)
        {
            switch (resourceType)
            {
            case RTYPE_Constant:
            case RTYPE_ConstantBuffer:
                return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            case RTYPE_Buffer:
            case RTYPE_BufferRaw:
            case RTYPE_Texture:
                return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            case RTYPE_RWBuffer:
            case RTYPE_RWBufferRaw:
            case RTYPE_RWTexture:
            case RTYPE_RWRenderTarget:
                return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            case RTYPE_Sampler:
                return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            }

            return (D3D12_DESCRIPTOR_RANGE_TYPE)0;
        }

        static void UpdateSubresourcesManual(
            ID3D12Device* device,
            ID3D12GraphicsCommandList* commandList,
            ID3D12Resource* destinationResource,
            ID3D12Resource* intermediateResource,
            uint64_t intermediateOffset,
            uint32_t firstSubresource,
            uint32_t numSubresources,
            D3D12_SUBRESOURCE_DATA* subresourceData)
        {
            D3D12_RESOURCE_DESC resourceDesc = destinationResource->GetDesc();

            std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
            std::vector<uint64_t> rowSizesInBytes(numSubresources);
            std::vector<uint32_t> numRows(numSubresources);
            uint64_t requiredSize = 0;
            device->GetCopyableFootprints(&resourceDesc, firstSubresource, numSubresources, intermediateOffset,
                                          layouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

            uint8_t* pData;
            intermediateResource->Map(0, nullptr, reinterpret_cast<void**>(&pData));
            for (uint32_t i = 0; i < numSubresources; ++i)
            {
                uint8_t* pDestSlice = pData + layouts[i].Offset;
                
                for (uint32_t y = 0; y < numRows[i]; ++y)
                {
                    memcpy(
                        pDestSlice + y * layouts[i].Footprint.RowPitch,
                        reinterpret_cast<const uint8_t*>(subresourceData[i].pData) + y * subresourceData[i].RowPitch,
                        rowSizesInBytes[i]
                    );
                }
            }
            intermediateResource->Unmap(0, nullptr);

            for (uint32_t i = 0; i < numSubresources; ++i)
            {
                D3D12_TEXTURE_COPY_LOCATION destLocation = {};
                destLocation.pResource = destinationResource;
                destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                destLocation.SubresourceIndex = i;

                D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
                srcLocation.pResource = intermediateResource;
                srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                srcLocation.PlacedFootprint = layouts[i];

                commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
            }
        }
    };
}
