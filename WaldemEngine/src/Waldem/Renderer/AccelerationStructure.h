#pragma once
#include "Buffer.h"
#include "GraphicTypes.h"

namespace Waldem
{
    class RayTracingGeometry
    {
    public:
        RayTracingGeometry() = default;
        RayTracingGeometry(Buffer* vertexBuffer, Buffer* indexBuffer) : VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer) {}
        virtual ~RayTracingGeometry() {}
        
        Buffer* VertexBuffer = nullptr;
        Buffer* IndexBuffer = nullptr;
    };
    
    class RayTracingInstance;
    
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
        Buffer* InstanceBuffer = nullptr;
    };
    
    class RayTracingInstance
    {
    public:
        RayTracingInstance() : BLAS(nullptr), Transform(Matrix4(1.0f)) {}
        RayTracingInstance(AccelerationStructure* blas, Matrix4 transform) : BLAS(blas), Transform(transform) {}
        virtual ~RayTracingInstance() {}

        AccelerationStructure* BLAS;
        Matrix4 Transform;
    };
}