#pragma once

namespace Waldem
{
    enum FillMode
    {
        WD_FILL_MODE_WIREFRAME	= 2,
        WD_FILL_MODE_SOLID	= 3
    };
    
    enum CullMode
    {
        WD_CULL_MODE_NONE	= 1,
        WD_CULL_MODE_FRONT	= 2,
        WD_CULL_MODE_BACK	= 3
    };
    
    enum ConservativeRasterizationMode
    {
        WD_CONSERVATIVE_RASTERIZATION_MODE_OFF	= 0,
        WD_CONSERVATIVE_RASTERIZATION_MODE_ON	= 1
    };
    
    struct DepthStencilDesc
    {
    };
    
    struct RasterizerDesc
    {
        FillMode FillMode = WD_FILL_MODE_SOLID;
        CullMode CullMode = WD_CULL_MODE_BACK;
        BOOL FrontCounterClockwise = FALSE;
        INT DepthBias;
        FLOAT DepthBiasClamp;
        FLOAT SlopeScaledDepthBias;
        BOOL DepthClipEnable;
        BOOL MultisampleEnable;
        BOOL AntialiasedLineEnable;
        UINT ForcedSampleCount;
        ConservativeRasterizationMode ConservativeRaster;
    };
    
    struct BlendDesc
    {
    };
    
    struct StreamOutputDesc
    {
    };
    
    struct SampleDesc
    {
    };
    
    struct InputLayoutDesc
    {
    };

    enum PrimitiveTopologyType
    {
        WD_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED	= 0,
        WD_PRIMITIVE_TOPOLOGY_TYPE_POINT	= 1,
        WD_PRIMITIVE_TOPOLOGY_TYPE_LINE	= 2,
        WD_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE	= 3,
        WD_PRIMITIVE_TOPOLOGY_TYPE_PATCH	= 4
    };
    
    enum ResourceStates
    {
        COMMON	= 0,
        VERTEX_AND_CONSTANT_BUFFER	= 0x1,
        INDEX_BUFFER	= 0x2,
        RENDER_TARGET	= 0x4,
        UNORDERED_ACCESS	= 0x8,
        DEPTH_WRITE	= 0x10,
        DEPTH_READ	= 0x20,
        NON_PIXEL_SHADER_RESOURCE	= 0x40,
        PIXEL_SHADER_RESOURCE	= 0x80,
        STREAM_OUT	= 0x100,
        INDIRECT_ARGUMENT	= 0x200,
        COPY_DEST	= 0x400,
        COPY_SOURCE	= 0x800,
        RESOLVE_DEST	= 0x1000,
        RESOLVE_SOURCE	= 0x2000,
        RAYTRACING_ACCELERATION_STRUCTURE	= 0x400000,
        SHADING_RATE_SOURCE	= 0x1000000,
        RESERVED_INTERNAL_8000	= 0x8000,
        RESERVED_INTERNAL_4000	= 0x4000,
        RESERVED_INTERNAL_100000	= 0x100000,
        RESERVED_INTERNAL_40000000	= 0x40000000,
        RESERVED_INTERNAL_80000000	= 0x80000000,
        READ_GENERIC	= ( ( ( ( ( 0x1 | 0x2 )  | 0x40 )  | 0x80 )  | 0x200 )  | 0x800 ) ,
        ALL_SHADER_RESOURCE	= ( 0x40 | 0x80 ) ,
        PRESENT	= 0,
        PREDICATION	= 0x200,
        VIDEO_DECODE_READ	= 0x10000,
        VIDEO_DECODE_WRITE	= 0x20000,
        VIDEO_PROCESS_READ	= 0x40000,
        VIDEO_PROCESS_WRITE	= 0x80000,
        VIDEO_ENCODE_READ	= 0x200000,
        VIDEO_ENCODE_WRITE	= 0x800000
    };
}
