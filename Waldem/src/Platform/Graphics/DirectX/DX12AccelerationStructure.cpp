#include "wdpch.h"
#include "DX12AccelerationStructure.h"

#include "D3DX12.h"
#include "DX12Buffer.h"

namespace Waldem
{
    ID3D12Resource* CreateBuffer(ID3D12Device* device, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, D3D12_HEAP_TYPE heapType)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = heapType;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = size;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = flags;

        ID3D12Resource* buffer;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initState, nullptr, IID_PPV_ARGS(&buffer));
        return buffer;
    }

    //BOTTOM LEVEL
    DX12AccelerationStructure::DX12AccelerationStructure(String name, ID3D12Device5* device, DX12CommandList* cmdList, AccelerationStructureType type, WArray<RayTracingGeometry>& geometries) : AccelerationStructure(name, type)
    {
        WArray<D3D12_RAYTRACING_GEOMETRY_DESC> dx12Geometries;

        for (auto& geometry : geometries)
        {
            D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
            geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            auto vertexBuffer = (DX12Buffer*)geometry.VertexBuffer;
            auto indexBuffer = (DX12Buffer*)geometry.IndexBuffer;
            
            auto vertexBufferGPUAddress = vertexBuffer->GetDefaultResource()->GetGPUVirtualAddress();
            auto indexBufferGPUAddress = indexBuffer->GetDefaultResource()->GetGPUVirtualAddress();
            
            geomDesc.Triangles.VertexBuffer.StartAddress = vertexBufferGPUAddress;
            geomDesc.Triangles.VertexBuffer.StrideInBytes = vertexBuffer->GetStride();
            geomDesc.Triangles.VertexCount = vertexBuffer->GetCount();
            geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geomDesc.Triangles.IndexBuffer = indexBufferGPUAddress;
            geomDesc.Triangles.IndexCount = indexBuffer->GetCount();
            geomDesc.Triangles.IndexFormat = indexBuffer->GetStride() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            geomDesc.Triangles.Transform3x4 = 0; //No transform for the BLAS, better to use instances' transforms
            
            dx12Geometries.Add(geomDesc);
        }
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
        asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        asInputs.NumDescs = static_cast<UINT>(dx12Geometries.Num());
        asInputs.pGeometryDescs = dx12Geometries.GetData();
        asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &prebuildInfo);

        prebuildInfo.ScratchDataSizeInBytes = (prebuildInfo.ScratchDataSizeInBytes + 255) & ~255;
        prebuildInfo.ResultDataMaxSizeInBytes = (prebuildInfo.ResultDataMaxSizeInBytes + 255) & ~255;

        ScratchBuffer = CreateBuffer(device, prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT);
        Resource = CreateBuffer(device, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_HEAP_TYPE_DEFAULT);
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
        asDesc.DestAccelerationStructureData = Resource->GetGPUVirtualAddress();

        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        cmdList->UAVBarrier(Resource);
    }

    //TOP LEVEL
    DX12AccelerationStructure::DX12AccelerationStructure(String name, ID3D12Device5* device, DX12CommandList* cmdList, AccelerationStructureType type, WArray<RayTracingInstance>& instances) : AccelerationStructure(name, type)
    {
        WArray<D3D12_RAYTRACING_INSTANCE_DESC> dx12Instances;

        for(int i = 0; i < instances.Num(); i++)
        {
            auto& instance = instances[i];
            
            D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
            instanceDesc.InstanceID = static_cast<UINT>(i);
            instanceDesc.InstanceMask = 0xFF;
            instanceDesc.InstanceContributionToHitGroupIndex = 0;
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            Matrix4 transposedMatrix = transpose(instance.Transform);
            memcpy(instanceDesc.Transform, &transposedMatrix, sizeof(instanceDesc.Transform));

            ID3D12Resource* blas = (ID3D12Resource*)((DX12AccelerationStructure*)instance.BLAS)->GetPlatformResource();
            instanceDesc.AccelerationStructure = blas->GetGPUVirtualAddress();
            
            dx12Instances.Add(instanceDesc);
        }
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
        asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        asInputs.NumDescs = static_cast<UINT>(dx12Instances.Num());
        asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

        InstanceBuffer = CreateBuffer(device, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * dx12Instances.Num(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
        void* mappedData;
        InstanceBuffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, dx12Instances.GetData(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * dx12Instances.Num());
        InstanceBuffer->Unmap(0, nullptr);

        asInputs.InstanceDescs = InstanceBuffer->GetGPUVirtualAddress();

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        device->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &prebuildInfo);

        prebuildInfo.ScratchDataSizeInBytes = (prebuildInfo.ScratchDataSizeInBytes + 255) & ~255;
        prebuildInfo.ResultDataMaxSizeInBytes = (prebuildInfo.ResultDataMaxSizeInBytes + 255) & ~255;

        ScratchBuffer = CreateBuffer(device, prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT);
        Resource = CreateBuffer(device, prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_HEAP_TYPE_DEFAULT);
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
        asDesc.DestAccelerationStructureData = Resource->GetGPUVirtualAddress();

        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(Resource);
        cmdList->ResourceBarrier(1, &barrier);
    }

    //BOTTOM LEVEL
    void DX12AccelerationStructure::Update(DX12CommandList* cmdList, WArray<RayTracingGeometry>& geometries)
    {
        // Update geometry descriptions
        WArray<D3D12_RAYTRACING_GEOMETRY_DESC> dx12Geometries;
        for (auto& geometry : geometries)
        {
            D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
            geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            auto vertexBuffer = (DX12Buffer*)geometry.VertexBuffer;
            auto indexBuffer = (DX12Buffer*)geometry.IndexBuffer;
            auto vertexBufferGPUAddress = vertexBuffer->GetDefaultResource()->GetGPUVirtualAddress();
            auto indexBufferGPUAddress = indexBuffer->GetDefaultResource()->GetGPUVirtualAddress();

            geomDesc.Triangles.VertexBuffer.StartAddress = vertexBufferGPUAddress;
            geomDesc.Triangles.VertexBuffer.StrideInBytes = vertexBuffer->GetStride();
            geomDesc.Triangles.VertexCount = vertexBuffer->GetCount();
            geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geomDesc.Triangles.IndexBuffer = indexBufferGPUAddress;
            geomDesc.Triangles.IndexCount = indexBuffer->GetCount();
            geomDesc.Triangles.IndexFormat = indexBuffer->GetStride() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            geomDesc.Triangles.Transform3x4 = 0;

            dx12Geometries.Add(geomDesc);
        }

        // Acceleration structure inputs
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
        asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        asInputs.NumDescs = static_cast<UINT>(dx12Geometries.Num());
        asInputs.pGeometryDescs = dx12Geometries.GetData();
        asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

        // Build the updated acceleration structure
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
        asDesc.DestAccelerationStructureData = Resource->GetGPUVirtualAddress();
        asDesc.SourceAccelerationStructureData = Resource->GetGPUVirtualAddress();  // Using existing BLAS as source

        // Issue the command to update the BLAS
        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        // Add a UAV barrier to ensure the structure update is complete
        cmdList->UAVBarrier(Resource);
    }


    //TOP LEVEL
    void DX12AccelerationStructure::Update(DX12CommandList* cmdList, WArray<RayTracingInstance>& instances)
    {
        // Update instance descriptors
        WArray<D3D12_RAYTRACING_INSTANCE_DESC> dx12Instances;
        for (int i = 0; i < instances.Num(); i++)
        {
            auto& instance = instances[i];

            D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
            instanceDesc.InstanceID = static_cast<UINT>(i);
            instanceDesc.InstanceMask = 0xFF;
            instanceDesc.InstanceContributionToHitGroupIndex = 0;
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
            Matrix4 transposedMatrix = transpose(instance.Transform);
            memcpy(instanceDesc.Transform, &transposedMatrix, sizeof(instanceDesc.Transform));

            ID3D12Resource* blas = (ID3D12Resource*)((DX12AccelerationStructure*)instance.BLAS)->GetPlatformResource();
            instanceDesc.AccelerationStructure = blas->GetGPUVirtualAddress();

            dx12Instances.Add(instanceDesc);
        }

        // Update the instance buffer with new transforms
        void* mappedData;
        InstanceBuffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, dx12Instances.GetData(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * dx12Instances.Num());
        InstanceBuffer->Unmap(0, nullptr);

        // Prepare the acceleration structure inputs for update
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
        asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        asInputs.NumDescs = static_cast<UINT>(dx12Instances.Num());
        asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;  // Important for refit
        asInputs.InstanceDescs = InstanceBuffer->GetGPUVirtualAddress();

        // Describe the acceleration structure build for update
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
        asDesc.DestAccelerationStructureData = Resource->GetGPUVirtualAddress();
        asDesc.SourceAccelerationStructureData = Resource->GetGPUVirtualAddress();  // Source is the existing TLAS

        // Build the updated TLAS
        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        // Insert a barrier to ensure completion
        auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(Resource);
        cmdList->ResourceBarrier(1, &barrier);
    }
}
