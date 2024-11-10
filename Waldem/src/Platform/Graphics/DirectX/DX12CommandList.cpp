#include "wdpch.h"
#include "DX12CommandList.h"
#include "D3DX12.h"
#include "DX12Buffer.h"
#include "DX12Helper.h"
#include "DX12PixelShader.h"
#include "DX12RenderTarget.h"

namespace Waldem
{
    DX12CommandList::DX12CommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        Device = device;
        
        //create the command allocator
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&CommandAllocator));
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command allocator!");
        }

        // Create the command list
        hr = device->CreateCommandList(0, type, CommandAllocator, nullptr, IID_PPV_ARGS(&CommandList));
        
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
    }

    DX12CommandList::~DX12CommandList()
    {
        CloseHandle(FenceEvent);
    }

    void DX12CommandList::Begin(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle)
    {
        CommandList->RSSetViewports(1, viewport);
        CommandList->RSSetScissorRects(1, scissor);

        CurrentViewport = *viewport;
        CurrentScissorRect = *scissor;

        //set render target
        CommandList->OMSetRenderTargets(1, &renderTargetHandle, FALSE, &depthStencilHandle);

        CurrentRenderTargetHandle = renderTargetHandle;
        CurrentDepthStencilHandle = depthStencilHandle;
    }

    void DX12CommandList::End()
    {
        Close();
    }

    void DX12CommandList::AddDrawCommand(Model* model, PixelShader* shader)
    {
        auto dxShader = (DX12PixelShader*)shader;
        auto pipeline = dxShader->GetPipeline();
        auto rootSignature = dxShader->GetRootSignature();
        auto resourcesHeap = dxShader->GetResourcesHeap();
        auto rootParams = dxShader->GetRootParams();

        //If shader does have its own render target, set it
        if(shader->RenderTarget)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle = ((DX12RenderTarget*)shader->RenderTarget)->GetRenderTargetHandle();

            D3D12_VIEWPORT Viewport = {};
            D3D12_RECT ScissorRect = {};
            
            Viewport.Width = shader->RenderTarget->GetWidth();
            Viewport.Height = shader->RenderTarget->GetHeight();
            Viewport.MinDepth = 0;
            Viewport.MaxDepth = 1;
            
            ScissorRect.right = shader->RenderTarget->GetWidth();
            ScissorRect.bottom = shader->RenderTarget->GetHeight();
            
            CommandList->RSSetViewports(1, &Viewport);
            CommandList->RSSetScissorRects(1, &ScissorRect);

            if(shader->RenderTarget->IsDepthStencilBuffer())
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);
                
                CommandList->OMSetRenderTargets(0, nullptr, FALSE, &renderTargetHandle);
		        CommandList->ClearDepthStencilView(renderTargetHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
            }
            else
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);

                CommandList->OMSetRenderTargets(1, &renderTargetHandle, FALSE, nullptr);
                const float clearColorFloat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		        CommandList->ClearRenderTargetView(renderTargetHandle, clearColorFloat, 0, 0);
            }
        }
        
        CommandList->SetPipelineState(pipeline);
        CommandList->SetGraphicsRootSignature(rootSignature);
        // auto samplersHeap = ((DX12PixelShader*)shader)->GetSamplersHeap();
        // ID3D12DescriptorHeap* heaps[] = { resourcesHeap, samplersHeap };
        // commandList->SetDescriptorHeaps(2, heaps);
        ID3D12DescriptorHeap* heaps[] = { resourcesHeap };
        CommandList->SetDescriptorHeaps(1, heaps);
        
        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = resourcesHeap->GetGPUDescriptorHandleForHeapStart();

        for (uint32_t i = 0; i < rootParams.size(); ++i)
        {
            CommandList->SetGraphicsRootDescriptorTable(i, handle);
            handle.ptr += descriptorSize;
        }
        
        CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //Draw meshes
        auto meshes = model->GetMeshes();

        for (auto mesh : meshes)
        {
            auto& indexBufferView = ((DX12IndexBuffer*)mesh->IB)->GetBufferView();
            auto& vertexBufferView = ((DX12VertexBuffer*)mesh->VB)->GetBufferView();
            uint32_t indexCountPerInstance = mesh->IB->GetCount();
            CommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            CommandList->IASetIndexBuffer(&indexBufferView);
            CommandList->DrawIndexedInstanced(mesh->IB->GetCount(), 1, 0, 0, 0);
        }

        //Set previous render target back
        if(shader->RenderTarget)
        {
            CommandList->RSSetViewports(1, &CurrentViewport);
            CommandList->RSSetScissorRects(1, &CurrentScissorRect);
            
            if(shader->RenderTarget->IsDepthStencilBuffer())
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);
            }
            else
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);
            }

            CommandList->OMSetRenderTargets(1, &CurrentRenderTargetHandle, FALSE, &CurrentDepthStencilHandle);
        }
    }

    void DX12CommandList::AddDrawCommand(Mesh* mesh, PixelShader* shader)
    {
        auto dxShader = (DX12PixelShader*)shader;
        auto pipeline = dxShader->GetPipeline();
        auto rootSignature = dxShader->GetRootSignature();
        auto resourcesHeap = dxShader->GetResourcesHeap();
        auto rootParams = dxShader->GetRootParams();

        //If shader does have its own render target, set it
        if(shader->RenderTarget)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle = ((DX12RenderTarget*)shader->RenderTarget)->GetRenderTargetHandle();

            D3D12_VIEWPORT Viewport = {};
            D3D12_RECT ScissorRect = {};
            
            Viewport.Width = shader->RenderTarget->GetWidth();
            Viewport.Height = shader->RenderTarget->GetHeight();
            ScissorRect.right = shader->RenderTarget->GetWidth();
            ScissorRect.bottom = shader->RenderTarget->GetHeight();
            
            CommandList->RSSetViewports(1, &Viewport);
            CommandList->RSSetScissorRects(1, &ScissorRect);

            if(shader->RenderTarget->IsDepthStencilBuffer())
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);
                
                CommandList->OMSetRenderTargets(0, nullptr, FALSE, &renderTargetHandle);
		        CommandList->ClearDepthStencilView(renderTargetHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
            }
            else
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);

                CommandList->OMSetRenderTargets(1, &renderTargetHandle, FALSE, nullptr);
                const float clearColorFloat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		        CommandList->ClearRenderTargetView(renderTargetHandle, clearColorFloat, 0, 0);
            }
        }
        
        CommandList->SetPipelineState(pipeline);
        CommandList->SetGraphicsRootSignature(rootSignature);
        // auto samplersHeap = ((DX12PixelShader*)shader)->GetSamplersHeap();
        // ID3D12DescriptorHeap* heaps[] = { resourcesHeap, samplersHeap };
        // commandList->SetDescriptorHeaps(2, heaps);
        ID3D12DescriptorHeap* heaps[] = { resourcesHeap };
        CommandList->SetDescriptorHeaps(1, heaps);
        
        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = resourcesHeap->GetGPUDescriptorHandleForHeapStart();

        for (uint32_t i = 0; i < rootParams.size(); ++i)
        {
            CommandList->SetGraphicsRootDescriptorTable(i, handle);
            handle.ptr += descriptorSize;
        }
        
        CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //Draw mesh
        auto& indexBufferView = ((DX12IndexBuffer*)mesh->IB)->GetBufferView();
        auto& vertexBufferView = ((DX12VertexBuffer*)mesh->VB)->GetBufferView();
        uint32_t indexCountPerInstance = mesh->IB->GetCount();
        CommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        CommandList->IASetIndexBuffer(&indexBufferView);
        CommandList->DrawIndexedInstanced(mesh->IB->GetCount(), 1, 0, 0, 0);

        //Set previous render target back
        if(shader->RenderTarget)
        {
            CommandList->RSSetViewports(1, &CurrentViewport);
            CommandList->RSSetScissorRects(1, &CurrentScissorRect);
            
            if(shader->RenderTarget->IsDepthStencilBuffer())
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);
            }
            else
            {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = (ID3D12Resource*)shader->RenderTarget->GetPlatformResource();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                CommandList->ResourceBarrier(1, &barrier);
            }

            CommandList->OMSetRenderTargets(1, &CurrentRenderTargetHandle, FALSE, &CurrentDepthStencilHandle);
        }
    }

    void DX12CommandList::Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil, Vector3 clearColor)
    {
        //clear the render target
        const float clearColorFloat[] = { clearColor.x, clearColor.y, clearColor.z, 1.0f };
        CommandList->ClearRenderTargetView(renderTarget, clearColorFloat, 0, nullptr);
        CommandList->ClearDepthStencilView(depthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
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

    void DX12CommandList::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox)
    {
        CommandList->CopyTextureRegion(dst, dstX, dstY, dstZ, src, srcBox);
    }

    void DX12CommandList::UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
    {
        UpdateSubresources(CommandList, destResource, srcResource, 0, 0, numSubresources, subresourceData);
    }

    void DX12CommandList::Execute(void* commandQueue)
    {
        ID3D12CommandQueue* dx12CommandQueue = static_cast<ID3D12CommandQueue*>(commandQueue);
        
        //execute the command list
        ID3D12CommandList* ppCommandLists[] = { CommandList };
        dx12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        //signal the fence after the GPU completes executing this command list
        HRESULT hr = dx12CommandQueue->Signal(Fence, ++FenceValue);
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to signal command queue fence!");
        }
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
