#pragma once
#include "Buffer.h"
#include "GraphicTypes.h"

namespace Waldem
{
    class RayTracingGeometry
    {
    public:
        RayTracingGeometry(Buffer* vertexBuffer, Buffer* indexBuffer) : VertexBuffer(vertexBuffer), IndexBuffer(indexBuffer) {}
        virtual ~RayTracingGeometry() {}
        
        Buffer* VertexBuffer;
        Buffer* IndexBuffer;
    };
    
    class RayTracingInstance;
    
    class AccelerationStructure
    {
    public:
        AccelerationStructure(String name, AccelerationStructureType type) : Name(name), Type(type) {}
        virtual ~AccelerationStructure() {}
        virtual String GetName() { return Name; }
        virtual AccelerationStructureType GetType() { return Type; }
        virtual void* GetPlatformResource() = 0;
        // virtual void Build() = 0;
        // virtual void Update() = 0;
    protected:
        String Name;
        AccelerationStructureType Type;
    };
    
    class RayTracingInstance
    {
    public:
        RayTracingInstance(AccelerationStructure* blas, Matrix4 transform) : BLAS(blas), Transform(transform) {}
        virtual ~RayTracingInstance() {}

        AccelerationStructure* BLAS;
        Matrix4 Transform;
    };
}