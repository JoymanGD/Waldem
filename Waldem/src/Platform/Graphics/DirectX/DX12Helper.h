#pragma once
#include <comdef.h>
#include "Waldem/Renderer/Shader.h"

namespace Waldem
{
    class WALDEM_API DX12Helper
    {
    public:
        static std::string MBFromW(LPCWSTR pwsz, UINT cp)
        {
            int cch = WideCharToMultiByte(cp, 0, pwsz, -1, 0, 0, NULL, NULL);

            char* psz = new char[cch];

            WideCharToMultiByte(cp, 0, pwsz, -1, psz, cch, NULL, NULL);

            std::string st(psz);
            delete[] psz;

            return st;
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

            std::string errorStr = "";

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
            case ConstantBuffer:
                return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            case Buffer:
            case BufferRaw:
            case Texture:
                return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            case RWBuffer:
            case RWBufferRaw:
            case RWTexture:
                return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            case Sampler:
                return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            }

            return (D3D12_DESCRIPTOR_RANGE_TYPE)0;
        }
    };
}
