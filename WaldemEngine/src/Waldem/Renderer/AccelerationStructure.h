#pragma once
#include "Buffer.h"
#include "GraphicTypes.h"
#include "Waldem/Types/MathTypes.h"

namespace Waldem
{
    class RayTracingGeometry
    {
    public:
        RayTracingGeometry() = default;
        RayTracingGeometry(Buffer* vertexBuffer, Buffer* indexBuffer, DrawCommand drawCommand, uint vertexCount) : VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer), DrawCommand(drawCommand), VertexCount(vertexCount) {}
        virtual ~RayTracingGeometry() {}
        
        Buffer* VertexBuffer = nullptr;
        Buffer* IndexBuffer = nullptr;
        DrawCommand DrawCommand = {0,1,0,0,0};
        uint VertexCount;
    };
    
    class AccelerationStructure : public GraphicResource
    {
    public:
        AccelerationStructure(WString name, AccelerationStructureType type) : Name(name), Type(type) { SetType(RTYPE_AccelerationStructure); }
        virtual ~AccelerationStructure() {}
        virtual WString GetName() { return Name; }
        virtual AccelerationStructureType GetType() { return Type; }
        Buffer* GetScratchBuffer() { return ScratchBuffer; }
        void SetScratchBuffer(Buffer* scratchBuffer) { ScratchBuffer = scratchBuffer; }
    protected:
        WString Name;
        AccelerationStructureType Type;
        Buffer* ScratchBuffer = nullptr;
    };
    
    struct RayTracingInstance
    {
        float Transform[ 3 ][ 4 ];
        uint InstanceID	: 24;
        uint InstanceMask : 8;
        uint InstanceContributionToHitGroupIndex : 24;
        uint Flags : 8;
        uint64 AccelerationStructure;
    };
}
