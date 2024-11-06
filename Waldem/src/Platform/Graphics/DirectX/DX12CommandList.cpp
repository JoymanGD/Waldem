#include "wdpch.h"
#include "DX12CommandList.h"

#include "DX12Buffer.h"
#include "DX12Helper.h"
#include "DX12PixelShader.h"

namespace Waldem
{
    DX12CommandList::DX12CommandList(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
    {
        //create the command allocator
        HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command allocator!");
        }

        // Create the command list
        hr = device->CreateCommandList(0, type, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command list!");
        }

        //command lists are created in the recording state. We need to close it initially.
        commandList->Close();
        
        //create the fence for synchronization
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create fence!");
        }

        //create an event handle for the fence
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        
        if (!fenceEvent)
        {
            throw std::runtime_error("Failed to create fence event!");
        }
    }

    DX12CommandList::~DX12CommandList()
    {
        CloseHandle(fenceEvent);
    }

    void DX12CommandList::Begin(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor, D3D12_CPU_DESCRIPTOR_HANDLE renderTarget)
    {
        //record rendering commands
        commandList->RSSetViewports(1, viewport);
        commandList->RSSetScissorRects(1, scissor);

        //set render target
        commandList->OMSetRenderTargets(1, &renderTarget, FALSE, nullptr);
    }

    void DX12CommandList::End()
    {
        Close();
    }

    void DX12CommandList::AddDrawCommand(Mesh* mesh, PixelShader* shader)
    {
        auto& indexBufferView = ((DX12IndexBuffer*)mesh->IB)->GetBufferView();
        auto& vertexBufferView = ((DX12VertexBuffer*)mesh->VB)->GetBufferView();
        auto pipeline = ((DX12PixelShader*)shader)->GetPipeline();
        auto rootSignature = ((DX12PixelShader*)shader)->GetRootSignature();
        auto resourcesHeap = ((DX12PixelShader*)shader)->GetResourcesHeap();

        commandList->SetPipelineState(pipeline);
        commandList->SetGraphicsRootSignature(rootSignature);
        // auto samplersHeap = ((DX12PixelShader*)shader)->GetSamplersHeap();
        // ID3D12DescriptorHeap* heaps[] = { resourcesHeap, samplersHeap };
        // commandList->SetDescriptorHeaps(2, heaps);
        ID3D12DescriptorHeap* heaps[] = { resourcesHeap };
        commandList->SetDescriptorHeaps(1, heaps);
        commandList->SetGraphicsRootDescriptorTable(0, resourcesHeap->GetGPUDescriptorHandleForHeapStart());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        commandList->IASetIndexBuffer(&indexBufferView);

        commandList->DrawIndexedInstanced(mesh->IB->GetCount(), 1, 0, 0, 0);
    }

    void DX12CommandList::Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, Vector3 clearColor)
    {
        //clear the render target
        const float clearColorFloat[] = { clearColor.x, clearColor.y, clearColor.z, 1.0f };
        commandList->ClearRenderTargetView(renderTarget, clearColorFloat, 0, nullptr);
    }

    void DX12CommandList::Reset()
    {
        //reset the command allocator (reuses the memory)
        HRESULT hr = commandAllocator->Reset();
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to reset command allocator!");
        }

        //reset the command list, ready to record commands
        hr = commandList->Reset(commandAllocator, nullptr); //pass a pipeline state object (PSO) if needed
        
        if (FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }
    }

    void DX12CommandList::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox)
    {
        commandList->CopyTextureRegion(dst, dstX, dstY, dstZ, src, srcBox);
    }

    void DX12CommandList::Execute(void* commandQueue)
    {
        ID3D12CommandQueue* dx12CommandQueue = static_cast<ID3D12CommandQueue*>(commandQueue);
        
        //execute the command list
        ID3D12CommandList* ppCommandLists[] = { commandList };
        dx12CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        //signal the fence after the GPU completes executing this command list
        HRESULT hr = dx12CommandQueue->Signal(fence, ++fenceValue);
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to signal command queue fence!");
        }
    }

    void DX12CommandList::WaitForCompletion()
    {
        //check if the GPU has finished executing the command list
        if (fence->GetCompletedValue() < fenceValue)
        {
            //if not, wait for the fence event to be signaled
            HRESULT hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
            
            if (FAILED(hr))
            {
                throw std::runtime_error("Failed to set fence event on completion!");
            }
            
            WaitForSingleObject(fenceEvent, INFINITE);  //wait for the event
        }
    }

    void DX12CommandList::ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier)
    {
        commandList->ResourceBarrier(count, barrier);
    }

    void DX12CommandList::Close()
    {
        //close the command list to indicate we're done recording commands.
        HRESULT hr = commandList->Close();
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to close command list!");
        }
    }
}
