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
    
    class AccelerationStructure
    {
    public:
        AccelerationStructure(WString name, AccelerationStructureType type) : Name(name), Type(type) {}
        virtual ~AccelerationStructure() {}
        virtual WString GetName() { return Name; }
        virtual AccelerationStructureType GetType() { return Type; }
        virtual void* GetPlatformResource() = 0;
        virtual void Destroy() = 0;
        // virtual void Build() = 0;
        // virtual void Update() = 0;
    protected:
        WString Name;
        AccelerationStructureType Type;
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