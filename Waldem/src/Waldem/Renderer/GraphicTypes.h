#pragma once

namespace Waldem
{
    enum WD_FILL_MODE
    {
        WD_FILL_MODE_WIREFRAME	= 2,
        WD_FILL_MODE_SOLID	= 3
    };
    
    enum WD_CULL_MODE
    {
        WD_CULL_MODE_NONE	= 1,
        WD_CULL_MODE_FRONT	= 2,
        WD_CULL_MODE_BACK	= 3
    };
    
    enum WD_CONSERVATIVE_RASTERIZATION_MODE
    {
        WD_CONSERVATIVE_RASTERIZATION_MODE_OFF	= 0,
        WD_CONSERVATIVE_RASTERIZATION_MODE_ON	= 1
    };
    
    struct WD_DEPTH_STENCIL_DESC
    {
    };
    
    struct WD_RASTERIZER_DESC
    {
        WD_FILL_MODE FillMode = WD_FILL_MODE_SOLID;
        WD_CULL_MODE CullMode = WD_CULL_MODE_BACK;
        BOOL FrontCounterClockwise = FALSE;
        INT DepthBias;
        FLOAT DepthBiasClamp;
        FLOAT SlopeScaledDepthBias;
        BOOL DepthClipEnable;
        BOOL MultisampleEnable;
        BOOL AntialiasedLineEnable;
        UINT ForcedSampleCount;
        WD_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster;
    };
    
    struct WD_BLEND_DESC
    {
    };
    
    struct WD_STREAM_OUTPUT_DESC
    {
    };
    
    struct WD_SAMPLE_DESC
    {
    };
    
    struct WD_INPUT_LAYOUT_DESC
    {
    };
    
    enum WD_TEXTURE_FORMAT
    {
        WD_FORMAT_UNKNOWN	                              = 0,
        WD_FORMAT_R32G32B32A32_TYPELESS                   = 1,
        WD_FORMAT_R32G32B32A32_FLOAT                      = 2,
        WD_FORMAT_R32G32B32A32_UINT                       = 3,
        WD_FORMAT_R32G32B32A32_SINT                       = 4,
        WD_FORMAT_R32G32B32_TYPELESS                      = 5,
        WD_FORMAT_R32G32B32_FLOAT                         = 6,
        WD_FORMAT_R32G32B32_UINT                          = 7,
        WD_FORMAT_R32G32B32_SINT                          = 8,
        WD_FORMAT_R16G16B16A16_TYPELESS                   = 9,
        WD_FORMAT_R16G16B16A16_FLOAT                      = 10,
        WD_FORMAT_R16G16B16A16_UNORM                      = 11,
        WD_FORMAT_R16G16B16A16_UINT                       = 12,
        WD_FORMAT_R16G16B16A16_SNORM                      = 13,
        WD_FORMAT_R16G16B16A16_SINT                       = 14,
        WD_FORMAT_R32G32_TYPELESS                         = 15,
        WD_FORMAT_R32G32_FLOAT                            = 16,
        WD_FORMAT_R32G32_UINT                             = 17,
        WD_FORMAT_R32G32_SINT                             = 18,
        WD_FORMAT_R32G8X24_TYPELESS                       = 19,
        WD_FORMAT_D32_FLOAT_S8X24_UINT                    = 20,
        WD_FORMAT_R32_FLOAT_X8X24_TYPELESS                = 21,
        WD_FORMAT_X32_TYPELESS_G8X24_UINT                 = 22,
        WD_FORMAT_R10G10B10A2_TYPELESS                    = 23,
        WD_FORMAT_R10G10B10A2_UNORM                       = 24,
        WD_FORMAT_R10G10B10A2_UINT                        = 25,
        WD_FORMAT_R11G11B10_FLOAT                         = 26,
        WD_FORMAT_R8G8B8A8_TYPELESS                       = 27,
        WD_FORMAT_R8G8B8A8_UNORM                          = 28,
        WD_FORMAT_R8G8B8A8_UNORM_SRGB                     = 29,
        WD_FORMAT_R8G8B8A8_UINT                           = 30,
        WD_FORMAT_R8G8B8A8_SNORM                          = 31,
        WD_FORMAT_R8G8B8A8_SINT                           = 32,
        WD_FORMAT_R16G16_TYPELESS                         = 33,
        WD_FORMAT_R16G16_FLOAT                            = 34,
        WD_FORMAT_R16G16_UNORM                            = 35,
        WD_FORMAT_R16G16_UINT                             = 36,
        WD_FORMAT_R16G16_SNORM                            = 37,
        WD_FORMAT_R16G16_SINT                             = 38,
        WD_FORMAT_R32_TYPELESS                            = 39,
        WD_FORMAT_D32_FLOAT                               = 40,
        WD_FORMAT_R32_FLOAT                               = 41,
        WD_FORMAT_R32_UINT                                = 42,
        WD_FORMAT_R32_SINT                                = 43,
        WD_FORMAT_R24G8_TYPELESS                          = 44,
        WD_FORMAT_D24_UNORM_S8_UINT                       = 45,
        WD_FORMAT_R24_UNORM_X8_TYPELESS                   = 46,
        WD_FORMAT_X24_TYPELESS_G8_UINT                    = 47,
        WD_FORMAT_R8G8_TYPELESS                           = 48,
        WD_FORMAT_R8G8_UNORM                              = 49,
        WD_FORMAT_R8G8_UINT                               = 50,
        WD_FORMAT_R8G8_SNORM                              = 51,
        WD_FORMAT_R8G8_SINT                               = 52,
        WD_FORMAT_R16_TYPELESS                            = 53,
        WD_FORMAT_R16_FLOAT                               = 54,
        WD_FORMAT_D16_UNORM                               = 55,
        WD_FORMAT_R16_UNORM                               = 56,
        WD_FORMAT_R16_UINT                                = 57,
        WD_FORMAT_R16_SNORM                               = 58,
        WD_FORMAT_R16_SINT                                = 59,
        WD_FORMAT_R8_TYPELESS                             = 60,
        WD_FORMAT_R8_UNORM                                = 61,
        WD_FORMAT_R8_UINT                                 = 62,
        WD_FORMAT_R8_SNORM                                = 63,
        WD_FORMAT_R8_SINT                                 = 64,
        WD_FORMAT_A8_UNORM                                = 65,
        WD_FORMAT_R1_UNORM                                = 66,
        WD_FORMAT_R9G9B9E5_SHAREDEXP                      = 67,
        WD_FORMAT_R8G8_B8G8_UNORM                         = 68,
        WD_FORMAT_G8R8_G8B8_UNORM                         = 69,
        WD_FORMAT_BC1_TYPELESS                            = 70,
        WD_FORMAT_BC1_UNORM                               = 71,
        WD_FORMAT_BC1_UNORM_SRGB                          = 72,
        WD_FORMAT_BC2_TYPELESS                            = 73,
        WD_FORMAT_BC2_UNORM                               = 74,
        WD_FORMAT_BC2_UNORM_SRGB                          = 75,
        WD_FORMAT_BC3_TYPELESS                            = 76,
        WD_FORMAT_BC3_UNORM                               = 77,
        WD_FORMAT_BC3_UNORM_SRGB                          = 78,
        WD_FORMAT_BC4_TYPELESS                            = 79,
        WD_FORMAT_BC4_UNORM                               = 80,
        WD_FORMAT_BC4_SNORM                               = 81,
        WD_FORMAT_BC5_TYPELESS                            = 82,
        WD_FORMAT_BC5_UNORM                               = 83,
        WD_FORMAT_BC5_SNORM                               = 84,
        WD_FORMAT_B5G6R5_UNORM                            = 85,
        WD_FORMAT_B5G5R5A1_UNORM                          = 86,
        WD_FORMAT_B8G8R8A8_UNORM                          = 87,
        WD_FORMAT_B8G8R8X8_UNORM                          = 88,
        WD_FORMAT_R10G10B10_XR_BIAS_A2_UNORM              = 89,
        WD_FORMAT_B8G8R8A8_TYPELESS                       = 90,
        WD_FORMAT_B8G8R8A8_UNORM_SRGB                     = 91,
        WD_FORMAT_B8G8R8X8_TYPELESS                       = 92,
        WD_FORMAT_B8G8R8X8_UNORM_SRGB                     = 93,
        WD_FORMAT_BC6H_TYPELESS                           = 94,
        WD_FORMAT_BC6H_UF16                               = 95,
        WD_FORMAT_BC6H_SF16                               = 96,
        WD_FORMAT_BC7_TYPELESS                            = 97,
        WD_FORMAT_BC7_UNORM                               = 98,
        WD_FORMAT_BC7_UNORM_SRGB                          = 99,
        WD_FORMAT_AYUV                                    = 100,
        WD_FORMAT_Y410                                    = 101,
        WD_FORMAT_Y416                                    = 102,
        WD_FORMAT_NV12                                    = 103,
        WD_FORMAT_P010                                    = 104,
        WD_FORMAT_P016                                    = 105,
        WD_FORMAT_420_OPAQUE                              = 106,
        WD_FORMAT_YUY2                                    = 107,
        WD_FORMAT_Y210                                    = 108,
        WD_FORMAT_Y216                                    = 109,
        WD_FORMAT_NV11                                    = 110,
        WD_FORMAT_AI44                                    = 111,
        WD_FORMAT_IA44                                    = 112,
        WD_FORMAT_P8                                      = 113,
        WD_FORMAT_A8P8                                    = 114,
        WD_FORMAT_B4G4R4A4_UNORM                          = 115,

        WD_FORMAT_P208                                    = 130,
        WD_FORMAT_V208                                    = 131,
        WD_FORMAT_V408                                    = 132,


        WD_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE         = 189,
        WD_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,

        WD_FORMAT_A4B4G4R4_UNORM                          = 191,


        WD_FORMAT_FORCE_UINT                  = 0xffffffff
    };
}
