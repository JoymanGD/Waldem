#include "wdpch.h"
#include "DX12CommandList.h"

#include <d3dcompiler.h>

#include "D3DX12.h"
#include "DX12Helper.h"
#include "DX12GraphicPipeline.h"
#include "DX12PixelShader.h"
#include "DX12RayTracingPipeline.h"
#include "Waldem/Renderer/Viewport.h"
#include "Waldem/Renderer/Model/Line.h"
#include "Waldem/Utils/FileUtils.h"

#define MAX_LINES 1000u

namespace Waldem
{
    DX12CommandList::DX12CommandList(ID3D12Device* device)
    {
        Device = device;
        
        //create the command allocator
        HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator));
        
        if (FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        // Create the command list
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocator, nullptr, IID_PPV_ARGS(&CommandList));
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command list!");
        }

        //command lists are created in the recording state. We need to close it initially.
        CommandList->Close();

        //create the fence for synchronization
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create fence!");
        }

        //create an event handle for the fence
        FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        
        if (!FenceEvent)
        {
            throw std::runtime_error("Failed to create fence event!");
        }
        
        Reset();
    }

    DX12CommandList::~DX12CommandList()
    {
        CloseHandle(FenceEvent);
    }

    void DX12CommandList::BeginInternal(SViewport& viewport, ID3D12DescriptorHeap* rtvHeap, ID3D12DescriptorHeap* dsvHeap)
    {
        //we use 0;0 for viewport and scissor rect position to render full-size render target
        D3D12_VIEWPORT d3d12Viewport = { 0, 0, (float)viewport.Size.x, (float)viewport.Size.y, (float)viewport.DepthRange.x, (float)viewport.DepthRange.y };
        D3D12_RECT d3d12ScissorRect = { 0, 0, viewport.Size.x, viewport.Size.y };
        CommandList->RSSetViewports(1, &d3d12Viewport);
        CommandList->RSSetScissorRects(1, &d3d12ScissorRect);

        uint32 index = viewport.FrameBuffer->GetCurrentRenderTarget()->GetIndex(RTV_DSV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += index * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        auto depth = viewport.FrameBuffer->GetDepth();

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = {};
        
        if(depth)
        {
            index = viewport.FrameBuffer->GetDepth()->GetIndex(RTV_DSV);
            dsv = dsvHeap->GetCPUDescriptorHandleForHeapStart();
            dsv.ptr += index * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }

        //set render target
        Clear(rtv, dsv, Vector3(0.0f, 0.0f, 0.0f));
        
        CommandList->OMSetRenderTargets(1, &rtv, FALSE, dsv.ptr > 0 ? &dsv : nullptr);
    }

    void DX12CommandList::EndInternal()
    {
        CurrentExecutableShader = nullptr;
    }

    void DX12CommandList::Draw(CMesh* mesh)
    {
        //Draw mesh
        if(mesh->VertexBuffer)
        {
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView { mesh->VertexBuffer->GetGPUAddress(), (uint)mesh->VertexBuffer->GetCapacity(), mesh->VertexBuffer->GetStride() };
            
            CommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        }

        if(mesh->IndexBuffer)
        {
            D3D12_INDEX_BUFFER_VIEW indexBufferView { mesh->IndexBuffer->GetGPUAddress(), (uint)mesh->IndexBuffer->GetCapacity(), DXGI_FORMAT_R32_UINT };
            
            CommandList->IASetIndexBuffer(&indexBufferView);
            CommandList->DrawIndexedInstanced(mesh->IndexBuffer->GetCount(), 1, 0, 0, 0);
        }
        else
        {
            CommandList->DrawInstanced(mesh->VertexBuffer->GetCount(), 1, 0, 0);
        }
    }

    void DX12CommandList::DrawIndexedInstanced(uint indexCount, uint instanceCount, uint startIndexLocation, int baseVertexLocation, uint startInstanceLocation)
    {
        CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
    }

    void DX12CommandList::Dispatch(Point3 groupCount)
    {
        CommandList->Dispatch(groupCount.x, groupCount.y, groupCount.z);
    }

    void DX12CommandList::TraceRays(Pipeline* rayTracingPipeline, Point3 numRays)
    {
        D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
        dispatchDesc.Width = numRays.x;
        dispatchDesc.Height = numRays.y;
        dispatchDesc.Depth = numRays.z;

        auto dx12Pipeline = (DX12RayTracingPipeline*)rayTracingPipeline;
        auto rayGenSBT = dx12Pipeline->GetRayGenSBT();
        auto missSBT = dx12Pipeline->GetMissSBT();
        auto hitGroupSBT = dx12Pipeline->GetHitGroupSBT();
        
        dispatchDesc.RayGenerationShaderRecord.StartAddress = rayGenSBT->Resource->GetGPUVirtualAddress();
        dispatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGenSBT->Size;
        
        dispatchDesc.MissShaderTable.StartAddress = missSBT->Resource->GetGPUVirtualAddress();
        dispatchDesc.MissShaderTable.SizeInBytes = missSBT->Size;

        dispatchDesc.HitGroupTable.StartAddress = hitGroupSBT->Resource->GetGPUVirtualAddress();
        dispatchDesc.HitGroupTable.SizeInBytes = hitGroupSBT->Size;
        
        CommandList->DispatchRays(&dispatchDesc);
    }

    void DX12CommandList::Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil, Vector3 clearColor)
    {
        //clear the render target
        const float clearColorFloat[] = { clearColor.x, clearColor.y, clearColor.z, 1.0f };
        CommandList->ClearRenderTargetView(renderTarget, clearColorFloat, 0, nullptr);
        CommandList->ClearDepthStencilView(depthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void DX12CommandList::SetPipeline(Pipeline* pipeline)
    {
        switch(pipeline->CurrentPipelineType)
        {
            case PipelineType::Graphics:
            {
                DX12GraphicPipeline* dx12Pipeline = (DX12GraphicPipeline*)pipeline;
                D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc = dx12Pipeline->GetDesc();

                D3D12_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
            
                switch (psoDesc->PrimitiveTopologyType)
                {
                case D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED:
                    break;
                case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
                    primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
                    break;
                case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
                    primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
                    break;
                case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
                    primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                    break;
                case D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH:
                    primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
                    break;
                }
                CommandList->IASetPrimitiveTopology(primitiveTopology);
                
                break;
            }
        }

        if(pipeline->CurrentPipelineType == PipelineType::RayTracing)
        {
            ID3D12StateObject* pso = (ID3D12StateObject*)pipeline->GetNativeObject();
            CommandList->SetPipelineState1(pso);
        }
        else
        {
            ID3D12PipelineState* pso = (ID3D12PipelineState*)pipeline->GetNativeObject();
            CommandList->SetPipelineState(pso);
        }
    }

    void DX12CommandList::SetRootSignature(ID3D12RootSignature* rootSignature)
    {
        CommandList->SetComputeRootSignature(rootSignature);
        CommandList->SetGraphicsRootSignature(rootSignature);
    }

    void DX12CommandList::SetGeneralDescriptorHeaps(ID3D12DescriptorHeap* resourcesHeap, ID3D12DescriptorHeap* samplersHeap)
    {
        ID3D12DescriptorHeap* heaps[] = { resourcesHeap, samplersHeap };
        SetDescriptorHeaps(_countof(heaps), &resourcesHeap);
    }

    void DX12CommandList::SetVertexBuffers(Buffer* vertexBuffer, uint32 numBuffers, uint32 startIndex)
    {
        //Draw mesh
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView { vertexBuffer->GetGPUAddress(), (uint)vertexBuffer->GetCapacity(), vertexBuffer->GetStride() };
            
        CommandList->IASetVertexBuffers(startIndex, numBuffers, &vertexBufferView);
    }

    void DX12CommandList::SetIndexBuffer(Buffer* indexBuffer)
    {
        D3D12_INDEX_BUFFER_VIEW indexBufferView { indexBuffer->GetGPUAddress(), (uint)indexBuffer->GetCapacity(), DXGI_FORMAT_R32_UINT };
            
        CommandList->IASetIndexBuffer(&indexBufferView);
    }

    void DX12CommandList::DrawIndirect(ID3D12CommandSignature* CommandSignature, uint numCommands, ID3D12Resource* indirectBuffer)
    {
        CommandList->ExecuteIndirect(CommandSignature, numCommands, indirectBuffer, 0, nullptr, 0);
    }

    void DX12CommandList::SetRenderTargets(WArray<D3D12_CPU_DESCRIPTOR_HANDLE>& renderTargets, D3D12_CPU_DESCRIPTOR_HANDLE* depthStencil)
    {
        WArray renderTargetHandlesToSet = { renderTargets };

        CommandList->OMSetRenderTargets(renderTargetHandlesToSet.Num(), renderTargetHandlesToSet.GetData(), FALSE, depthStencil);
    }

    void DX12CommandList::SetViewport(D3D12_VIEWPORT& viewport, D3D12_RECT& scissor)
    {
        CommandList->RSSetViewports(1, &viewport);
        CommandList->RSSetScissorRects(1, &scissor);
    }

    void DX12CommandList::SetDescriptorHeaps(uint32_t NumDescriptorHeaps, ID3D12DescriptorHeap* const* ppDescriptorHeaps)
    {
        CommandList->SetDescriptorHeaps(NumDescriptorHeaps, ppDescriptorHeaps);
    }

    void DX12CommandList::Reset()
    {
        //reset the command allocator (reuses the memory)
        HRESULT hr = CommandAllocator->Reset();
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to reset command allocator!");
        }

        //reset the command list, ready to record commands
        hr = CommandList->Reset(CommandAllocator, nullptr); //pass a pipeline state object (PSO) if needed
        
        if (FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }
    }

    void DX12CommandList::SetConstants(uint32_t rootParamIndex, uint32_t numConstants, void* data, PipelineType pipelineType)
    {
        if(pipelineType == PipelineType::Compute || pipelineType == PipelineType::RayTracing)
        {
            CommandList->SetComputeRoot32BitConstants(rootParamIndex, numConstants, data, 0);
        }
        else
        {
            CommandList->SetGraphicsRoot32BitConstants(rootParamIndex, numConstants, data, 0);
        }
    }

    void DX12CommandList::BuildRaytracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* asDesc)
    {
        CommandList->BuildRaytracingAccelerationStructure(asDesc, 0, nullptr);
    }

    void DX12CommandList::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox)
    {
        CommandList->CopyTextureRegion(dst, dstX, dstY, dstZ, src, srcBox);
    }

    void DX12CommandList::CopyResource(ID3D12Resource* dst, ID3D12Resource* src)
    {
        CommandList->CopyResource(dst, src);
    }

    void DX12CommandList::CopyBufferRegion(ID3D12Resource* dst, size_t destOffset, ID3D12Resource* src, size_t srcOffset, size_t size)
    {
        CommandList->CopyBufferRegion(dst, destOffset, src, srcOffset, size);
    }

    void DX12CommandList::UpdateRes(ID3D12Resource* resource, ID3D12Resource* uploadResource, void* data, uint32_t size, D3D12_RESOURCE_STATES beforeState, uint offset)
    {
        void* mappedData;
        uploadResource->Map(0, nullptr, &mappedData);
        memcpy(static_cast<uint8*>(mappedData) + offset, data, size);
        uploadResource->Unmap(0, nullptr);

        ResourceBarrier(resource, beforeState, D3D12_RESOURCE_STATE_COPY_DEST);

        CommandList->CopyBufferRegion(resource, offset, uploadResource, offset, size);

        ResourceBarrier(resource, D3D12_RESOURCE_STATE_COPY_DEST, beforeState);
    }

    void DX12CommandList::ClearRes(ID3D12Resource* resource, ID3D12Resource* uploadResource, uint32_t size, uint offset, D3D12_RESOURCE_STATES beforeState)
    {
        void* mappedData;
        uploadResource->Map(0, nullptr, &mappedData);
        memset(static_cast<uint8_t*>(mappedData) + offset, 0, size);
        uploadResource->Unmap(0, nullptr);

        ResourceBarrier(resource, beforeState, D3D12_RESOURCE_STATE_COPY_DEST);
        CommandList->CopyBufferRegion(resource, offset, uploadResource, offset, size);
        ResourceBarrier(resource, D3D12_RESOURCE_STATE_COPY_DEST, beforeState);
    }

    void DX12CommandList::UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
    {
        UpdateSubresources(CommandList, destResource, srcResource, 0, 0, numSubresources, subresourceData);
    }

    void DX12CommandList::Execute(ID3D12CommandQueue* commandQueue)
    {
        ID3D12CommandQueue* dx12CommandQueue = commandQueue;
        
        //execute the command list
        ID3D12CommandList* ppCommandLists[] = { CommandList };
        dx12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        //signal the fence after the GPU completes executing this command list
        HRESULT hr = dx12CommandQueue->Signal(Fence, ++FenceValue);
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to signal command queue fence!");
        }

        WaitForCompletion();
    }

    void DX12CommandList::WaitForCompletion()
    {
        //check if the GPU has finished executing the command list
        if (Fence->GetCompletedValue() < FenceValue)
        {
            //if not, wait for the fence event to be signaled
            HRESULT hr = Fence->SetEventOnCompletion(FenceValue, FenceEvent);
            
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to set fence event on completion!");
            }
            
            WaitForSingleObject(FenceEvent, INFINITE);  //wait for the event
        }
    }

    void DX12CommandList::ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier)
    {
        CommandList->ResourceBarrier(count, barrier);
    }

    void DX12CommandList::ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after; // Or your desired state
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        CommandList->ResourceBarrier(1, &barrier);
    }

    void DX12CommandList::UAVBarrier(ID3D12Resource* resource)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource);
        CommandList->ResourceBarrier(1, &barrier);
    }

    void DX12CommandList::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle)
    {
        const float clearColorFloat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        CommandList->ClearRenderTargetView(renderTargetHandle, clearColorFloat, 0, 0);
    }

    void DX12CommandList::ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle)
    {
        CommandList->ClearDepthStencilView(depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    }

    void DX12CommandList::Close()
    {
        //close the command list to indicate we're done recording commands.
        HRESULT hr = CommandList->Close();
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to close command list!");
        }
    }
}
