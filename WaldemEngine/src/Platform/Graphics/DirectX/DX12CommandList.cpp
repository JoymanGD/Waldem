#include "wdpch.h"
#include "DX12CommandList.h"

#include <d3dcompiler.h>

#include "D3DX12.h"
#include "DX12Buffer.h"
#include "DX12Helper.h"
#include "DX12GraphicPipeline.h"
#include "DX12PixelShader.h"
#include "DX12RayTracingPipeline.h"
#include "DX12RenderTarget.h"
#include "DX12RootSignature.h"
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
            throw std::runtime_error("Failed to create command allocator!");
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

    void DX12CommandList::BeginInternal(SViewport& viewport)
    {
        //we use 0;0 for viewport and scissor rect position to render full-size render target
        D3D12_VIEWPORT d3d12Viewport = { 0, 0, (float)viewport.Size.x, (float)viewport.Size.y, (float)viewport.DepthRange.x, (float)viewport.DepthRange.y };
        D3D12_RECT d3d12ScissorRect = { 0, 0, viewport.Size.x, viewport.Size.y };
        CommandList->RSSetViewports(1, &d3d12Viewport);
        CommandList->RSSetScissorRects(1, &d3d12ScissorRect);

        MainViewport = d3d12Viewport;
        MainScissorRect = d3d12ScissorRect;

        //set render target
        auto rtvHandle = ((DX12RenderTarget*)viewport.FrameBuffer->GetCurrentRenderTarget())->GetRTVHandle();
        auto depthStencilHandle = ((DX12RenderTarget*)viewport.FrameBuffer->GetDepth())->GetRTVHandle();
        
        Clear(rtvHandle, depthStencilHandle, Vector3(0.0f, 0.0f, 0.0f));
        
        CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &depthStencilHandle);

        MainRenderTargetHandles.Add(rtvHandle);
        MainDepthStencilHandle = depthStencilHandle;
    }

    void DX12CommandList::EndInternal()
    {
        MainRenderTargetHandles.Clear();
        CurrentExecutableShader = nullptr;
    }

    void DX12CommandList::Draw(CModel* model)
    {
        //Draw meshes
        auto meshes = model->GetMeshes();

        for (auto mesh : meshes)
        {
            Draw(mesh);
        }
    }

    void DX12CommandList::Draw(CMesh* mesh)
    {
        //Draw mesh
        if(mesh->VertexBuffer)
        {
            auto& vertexBufferView = ((DX12Buffer*)mesh->VertexBuffer)->GetVertexBufferView();
            CommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        }

        if(mesh->IndexBuffer)
        {
            auto& indexBufferView = ((DX12Buffer*)mesh->IndexBuffer)->GetIndexBufferView();
            CommandList->IASetIndexBuffer(&indexBufferView);
            CommandList->DrawIndexedInstanced(mesh->IndexBuffer->GetCount(), 1, 0, 0, 0);
        }
        else
        {
            CommandList->DrawInstanced(mesh->VertexBuffer->GetCount(), 1, 0, 0);
        }
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

    void DX12CommandList::SetRootSignature(RootSignature* rootSignature)
    {
        DX12RootSignature* dx12RootSignature = (DX12RootSignature*)rootSignature;
        ID3D12RootSignature* rootSignatureObject = (ID3D12RootSignature*)dx12RootSignature->GetNativeObject();

        if(rootSignature->CurrentPipelineType == PipelineType::Compute)
        {
            CommandList->SetComputeRootSignature(rootSignatureObject);
        }
        else
        {
            CommandList->SetGraphicsRootSignature(rootSignatureObject);
        }
        
        auto rootParamDatas = dx12RootSignature->GetRootParamDatas();

        WArray<ID3D12DescriptorHeap*> heaps;

        auto resourcesHeap = dx12RootSignature->GetResourcesHeap();
        if(resourcesHeap)
        {
            heaps.Add(resourcesHeap);
        }
        
        auto samplersHeap = dx12RootSignature->GetSamplersHeap();
        if(samplersHeap)
        {
            heaps.Add(samplersHeap);
        }

        if(!heaps.IsEmpty())
        {
            CommandList->SetDescriptorHeaps(heaps.Num(), heaps.GetData());
        }

        if(resourcesHeap)
        {
            UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_GPU_DESCRIPTOR_HANDLE handle = resourcesHeap->GetGPUDescriptorHandleForHeapStart();

            for (uint32_t i = 0; i < rootParamDatas.Num(); ++i)
            {
                auto& rootParamData = rootParamDatas[i];
                
                if(rootParamData.Type != RTYPE_Sampler && rootParamData.Type != RTYPE_Constant)
                {
                    if(rootSignature->CurrentPipelineType == PipelineType::Compute)
                    {
                        CommandList->SetComputeRootDescriptorTable(i, handle);
                    }
                    else
                    {
                        CommandList->SetGraphicsRootDescriptorTable(i, handle);
                    }
        
                    handle.ptr += descriptorSize * rootParamData.NumDescriptors;
                }
            }
        }

        if(samplersHeap)
        {
            UINT samplerDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            D3D12_GPU_DESCRIPTOR_HANDLE samplersHandle = samplersHeap->GetGPUDescriptorHandleForHeapStart();

            for (uint32_t i = 0; i < rootParamDatas.Num(); ++i)
            {
                auto& rootParamData = rootParamDatas[i];
        
                if(rootParamData.Type == RTYPE_Sampler)
                {
                    if(rootSignature->CurrentPipelineType == PipelineType::Compute)
                    {
                        CommandList->SetComputeRootDescriptorTable(i, samplersHandle);
                    }
                    else
                    {
                        CommandList->SetGraphicsRootDescriptorTable(i, samplersHandle);
                    }
        
                    samplersHandle.ptr += samplerDescriptorSize;
                }
            }
        }
    }

    void DX12CommandList::SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil)
    {
        WArray<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetHandlesToSet = MainRenderTargetHandles;
        D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandleToSet = MainDepthStencilHandle;
        D3D12_VIEWPORT viewportToSet = MainViewport;
        D3D12_RECT scissorToSet = MainScissorRect;

        if(!renderTargets.IsEmpty())
        {
            renderTargetHandlesToSet.Clear();
            
            for(auto renderTarget : renderTargets)
            {
                if(renderTarget != nullptr)
                {
                    auto handle = ((DX12RenderTarget*)renderTarget)->GetRTVHandle();
                    renderTargetHandlesToSet.Add(handle);
                }
            }
        }

        if(depthStencil)
        {
            depthStencilHandleToSet = ((DX12RenderTarget*)depthStencil)->GetRTVHandle();
        }

        // if(viewport.Width != 0.f && viewport.Height != 0.f)
        // {
        //     viewportToSet.Height = viewport.Height;
        //     viewportToSet.Width = viewport.Width;
        //     viewportToSet.MinDepth = viewport.MinDepth;
        //     viewportToSet.MaxDepth = viewport.MaxDepth;
        // }
        //
        // if(scissor.bottom != 0 && scissor.right != 0)
        // {
        //     scissorToSet = *(D3D12_RECT*)&scissor;
        // }
        
        CommandList->RSSetViewports(1, &viewportToSet);
        CommandList->RSSetScissorRects(1, &scissorToSet);
        CommandList->OMSetRenderTargets(renderTargetHandlesToSet.Num(), renderTargetHandlesToSet.GetData(), FALSE, &depthStencilHandleToSet);
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
        if(pipelineType == PipelineType::Compute)
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

    void DX12CommandList::CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT)
    {
        ID3D12Resource* dst = (ID3D12Resource*)dstRT->GetPlatformResource();
        ID3D12Resource* src = (ID3D12Resource*)srcRT->GetPlatformResource();
        CommandList->CopyResource(dst, src);
    }

    void DX12CommandList::CopyBuffer(Buffer* dstBuffer, Buffer* srcBuffer)
    {
        ID3D12Resource* dst = (ID3D12Resource*)dstBuffer->GetPlatformResource();
        ID3D12Resource* src = (ID3D12Resource*)srcBuffer->GetPlatformResource();
        CommandList->CopyResource(dst, src);
    }

    void DX12CommandList::UpdateBuffer(Buffer* buffer, void* data, uint32_t size)
    {
        DX12Buffer* dx12Buffer = (DX12Buffer*)buffer;
        ID3D12Resource* uploadResource = dx12Buffer->GetUploadResource();
        ID3D12Resource* defaultResource = dx12Buffer->GetDefaultResource();
        void* mappedData;
        uploadResource->Map(0, nullptr, &mappedData);
        memcpy(mappedData, data, size);
        uploadResource->Unmap(0, nullptr);

        ResourceBarrier(defaultResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

        CommandList->CopyBufferRegion(defaultResource, 0, uploadResource, 0, size);

        ResourceBarrier(defaultResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        buffer->SetCount(size / buffer->GetStride());
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
