#pragma once
#include "TextureFormat.h"
#include "Waldem/Types/String.h"

namespace Waldem
{    
    enum class PipelineType
    {
        Graphics = 0,
        Compute = 1,
        RayTracing = 2
    };
    
    enum class AccelerationStructureType
    {
        BottomLevel = 0,
        TopLevel = 1
    };
    
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
    
    enum DepthWriteMask
    {
        WD_DEPTH_WRITE_MASK_ZERO	= 0,
        WD_DEPTH_WRITE_MASK_ALL	= 1
    };
    
    enum ComparisonFunc
    {
        WD_COMPARISON_FUNC_NONE	= 0,
        WD_COMPARISON_FUNC_NEVER	= 1,
        WD_COMPARISON_FUNC_LESS	= 2,
        WD_COMPARISON_FUNC_EQUAL	= 3,
        WD_COMPARISON_FUNC_LESS_EQUAL	= 4,
        WD_COMPARISON_FUNC_GREATER	= 5,
        WD_COMPARISON_FUNC_NOT_EQUAL	= 6,
        WD_COMPARISON_FUNC_GREATER_EQUAL	= 7,
        WD_COMPARISON_FUNC_ALWAYS	= 8
    };

    enum StencilOp
    {
        WD_STENCIL_OP_KEEP	= 1,
        WD_STENCIL_OP_ZERO	= 2,
        WD_STENCIL_OP_REPLACE	= 3,
        WD_STENCIL_OP_INCR_SAT	= 4,
        WD_STENCIL_OP_DECR_SAT	= 5,
        WD_STENCIL_OP_INVERT	= 6,
        WD_STENCIL_OP_INCR	= 7,
        WD_STENCIL_OP_DECR	= 8
    };

    struct DepthStencilOpDesc
    {
        StencilOp StencilFailOp;
        StencilOp StencilDepthFailOp;
        StencilOp StencilPassOp;
        ComparisonFunc StencilFunc;
    };
    
    struct DepthStencilDesc
    {
        bool DepthEnable;
        DepthWriteMask DepthWriteMask;
        ComparisonFunc DepthFunc;
        bool StencilEnable;
        uint8_t StencilReadMask;
        uint8_t StencilWriteMask;
        DepthStencilOpDesc FrontFace;
        DepthStencilOpDesc BackFace;
    };
    #define DEFAULT_DEPTH_STENCIL_DESC { true, WD_DEPTH_WRITE_MASK_ALL, WD_COMPARISON_FUNC_LESS, false, 0, 0, { WD_STENCIL_OP_KEEP, WD_STENCIL_OP_KEEP, WD_STENCIL_OP_KEEP, WD_COMPARISON_FUNC_ALWAYS }, { WD_STENCIL_OP_KEEP, WD_STENCIL_OP_KEEP, WD_STENCIL_OP_KEEP, WD_COMPARISON_FUNC_ALWAYS } }
    
    struct RasterizerDesc
    {
        FillMode FillMode = WD_FILL_MODE_SOLID;
        CullMode CullMode = WD_CULL_MODE_BACK;
        bool FrontCounterClockwise = false;
        int DepthBias;
        float DepthBiasClamp;
        float SlopeScaledDepthBias;
        bool DepthClipEnable;
        bool MultisampleEnable;
        bool AntialiasedLineEnable;
        uint32_t ForcedSampleCount;
        ConservativeRasterizationMode ConservativeRaster;
    };
    #define DEFAULT_RASTERIZER_DESC { WD_FILL_MODE_SOLID, WD_CULL_MODE_BACK, false, 0, 0.0f, 0.0f, true, false, false, 0, ConservativeRasterizationMode::WD_CONSERVATIVE_RASTERIZATION_MODE_OFF }
    
    
    enum class Blend
    {
        ZERO = 1,
        ONE = 2,
        SRC_COLOR = 3,
        INV_SRC_COLOR = 4,
        SRC_ALPHA = 5,
        INV_SRC_ALPHA = 6,
        DEST_ALPHA = 7,
        INV_DEST_ALPHA = 8,
        DEST_COLOR = 9,
        INV_DEST_COLOR = 10,
    };

    enum class BlendOperation
    {
        ADD = 1,
        SUBTRACT = 2,
        REV_SUBTRACT = 3,
        MIN = 4,
        MAX = 5,
    };
    
    enum COLOR_WRITE_ENABLE
    {
        COLOR_WRITE_ENABLE_RED	= 1,
        COLOR_WRITE_ENABLE_GREEN	= 2,
        COLOR_WRITE_ENABLE_BLUE	= 4,
        COLOR_WRITE_ENABLE_ALPHA	= 8,
        COLOR_WRITE_ENABLE_ALL	= ( ( ( COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN )  | COLOR_WRITE_ENABLE_BLUE )  | COLOR_WRITE_ENABLE_ALPHA ) 
    };

    struct RenderTargetBlendDesc
    {
        bool BlendEnable = false;
        Blend SrcBlend = Blend::ONE;
        Blend DestBlend = Blend::ZERO;
        BlendOperation BlendOp = BlendOperation::ADD;
        Blend SrcBlendAlpha = Blend::ONE;
        Blend DestBlendAlpha = Blend::ZERO;
        BlendOperation BlendOpAlpha = BlendOperation::ADD;
        uint8_t RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;
    };

    struct BlendDesc
    {
        bool AlphaToCoverageEnable = false;
        bool IndependentBlendEnable = false;
        RenderTargetBlendDesc RenderTarget[8];
    };
    #define DEFAULT_BLEND_DESC { false, false, { { false, Blend::ONE, Blend::ZERO, BlendOperation::ADD, Blend::ONE, Blend::ZERO, BlendOperation::ADD, COLOR_WRITE_ENABLE_ALL } } }
    #define ALPHA_BLEND_DESC { false, false, { { true, Blend::SRC_ALPHA, Blend::INV_SRC_ALPHA, BlendOperation::ADD, Blend::ONE, Blend::ONE, BlendOperation::ADD, COLOR_WRITE_ENABLE_ALL } } }
    
    struct StreamOutputDesc
    {
    };
    
    struct SampleDesc
    {
    };

    enum InputClassification
    {
        WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA	= 0,
        WD_INPUT_CLASSIFICATION_PER_INSTANCE_DATA = 1
    };
    
    struct InputLayoutDesc
    {
        WString SemanticName;
        uint32_t SemanticIndex;
        TextureFormat Format;
        uint32_t InputSlot;
        uint32_t AlignedByteOffset;
        InputClassification InputSlotClass;
        uint32_t InstanceDataStepRate;
    };

    struct DrawCommand
    {
        uint IndexCountPerInstance;
        uint InstanceCount;
        uint StartIndexLocation;
        int BaseVertexLocation;
        uint StartInstanceLocation;
    };

    struct IndirectCommand
    {
        uint DrawId;
        DrawCommand DrawIndexed;
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

    inline ResourceStates operator|(ResourceStates a, ResourceStates b)
    {
        return static_cast<ResourceStates>(static_cast<int>(a) | static_cast<int>(b));
    }

    #define DEFAULT_INPUT_LAYOUT_DESC \
    {{ "POSITION", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 0, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, \
    { "COLOR", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 16, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, \
    { "NORMAL", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 32, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, \
    { "TANGENT", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 48, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, \
    { "BITANGENT", 0, TextureFormat::R32G32B32A32_FLOAT, 0, 64, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, \
    { "TEXCOORD", 0, TextureFormat::R32G32_FLOAT, 0, 80, WD_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }}
}
