#pragma once

namespace Waldem
{
    enum WD_SAMPLER_FILTER
    {
        MIN_MAG_MIP_POINT	= 0,
        MIN_MAG_POINT_MIP_LINEAR	= 0x1,
        MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x4,
        MIN_POINT_MAG_MIP_LINEAR	= 0x5,
        MIN_LINEAR_MAG_MIP_POINT	= 0x10,
        MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x11,
        MIN_MAG_LINEAR_MIP_POINT	= 0x14,
        MIN_MAG_MIP_LINEAR	= 0x15,
        MIN_MAG_ANISOTROPIC_MIP_POINT	= 0x54,
        ANISOTROPIC	= 0x55,
        COMPARISON_MIN_MAG_MIP_POINT	= 0x80,
        COMPARISON_MIN_MAG_POINT_MIP_LINEAR	= 0x81,
        COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x84,
        COMPARISON_MIN_POINT_MAG_MIP_LINEAR	= 0x85,
        COMPARISON_MIN_LINEAR_MAG_MIP_POINT	= 0x90,
        COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x91,
        COMPARISON_MIN_MAG_LINEAR_MIP_POINT	= 0x94,
        COMPARISON_MIN_MAG_MIP_LINEAR	= 0x95,
        COMPARISON_MIN_MAG_ANISOTROPIC_MIP_POINT	= 0xd4,
        COMPARISON_ANISOTROPIC	= 0xd5,
        MINIMUM_MIN_MAG_MIP_POINT	= 0x100,
        MINIMUM_MIN_MAG_POINT_MIP_LINEAR	= 0x101,
        MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x104,
        MINIMUM_MIN_POINT_MAG_MIP_LINEAR	= 0x105,
        MINIMUM_MIN_LINEAR_MAG_MIP_POINT	= 0x110,
        MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x111,
        MINIMUM_MIN_MAG_LINEAR_MIP_POINT	= 0x114,
        MINIMUM_MIN_MAG_MIP_LINEAR	= 0x115,
        MINIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT	= 0x154,
        MINIMUM_ANISOTROPIC	= 0x155,
        MAXIMUM_MIN_MAG_MIP_POINT	= 0x180,
        MAXIMUM_MIN_MAG_POINT_MIP_LINEAR	= 0x181,
        MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT	= 0x184,
        MAXIMUM_MIN_POINT_MAG_MIP_LINEAR	= 0x185,
        MAXIMUM_MIN_LINEAR_MAG_MIP_POINT	= 0x190,
        MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR	= 0x191,
        MAXIMUM_MIN_MAG_LINEAR_MIP_POINT	= 0x194,
        MAXIMUM_MIN_MAG_MIP_LINEAR	= 0x195,
        MAXIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT	= 0x1d4,
        MAXIMUM_ANISOTROPIC	= 0x1d5
    };

    enum WD_TEXTURE_ADDRESS_MODE
    {
        WRAP	= 1,
        MIRROR	= 2,
        CLAMP	= 3,
        BORDER	= 4,
        MIRROR_ONCE	= 5
    };

    enum WD_COMPARISON_FUNC
    {
        NONE	= 0,
        NEVER	= 1,
        LESS	= 2,
        EQUAL	= 3,
        LESS_EQUAL	= 4,
        GREATER	= 5,
        NOT_EQUAL	= 6,
        GREATER_EQUAL	= 7,
        ALWAYS	= 8
    };
    
    class WALDEM_API Sampler
    {
    public:
        WD_SAMPLER_FILTER Filter;
        WD_TEXTURE_ADDRESS_MODE AddressU;
        WD_TEXTURE_ADDRESS_MODE AddressV;
        WD_TEXTURE_ADDRESS_MODE AddressW;
        float MipLODBias = 0.0f;
        uint32_t MaxAnisotropy = 1;
        WD_COMPARISON_FUNC ComparisonFunc;
        float MinLOD = 0.0f;
        float MaxLOD = 3.402823466e+38f;

        Sampler(WD_SAMPLER_FILTER filter, WD_TEXTURE_ADDRESS_MODE addressU, WD_TEXTURE_ADDRESS_MODE addressV, WD_TEXTURE_ADDRESS_MODE addressW, float mipLODBias, uint32_t maxAnisotropy, WD_COMPARISON_FUNC comparisonFunc, float minLOD, float maxLOD)
            : Filter(filter), AddressU(addressU), AddressV(addressV), AddressW(addressW), MipLODBias(mipLODBias), MaxAnisotropy(maxAnisotropy), ComparisonFunc(comparisonFunc), MinLOD(minLOD), MaxLOD(maxLOD) {}
        
        Sampler(WD_SAMPLER_FILTER filter, WD_TEXTURE_ADDRESS_MODE addressU, WD_TEXTURE_ADDRESS_MODE addressV, WD_TEXTURE_ADDRESS_MODE addressW, WD_COMPARISON_FUNC comparisonFunc)
            : Filter(filter), AddressU(addressU), AddressV(addressV), AddressW(addressW), ComparisonFunc(comparisonFunc) {}
    };
}