#include "wdpch.h"
#include "DX12ComputeCommandList.h"
#include "D3DX12.h"
#include "DX12ComputeShader.h"
#include "DX12Helper.h"

namespace Waldem
{
    DX12ComputeCommandList::DX12ComputeCommandList(ID3D12Device* device)
    {
        Device = device;
        
        //create the command allocator
        HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&CommandAllocator));
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create command allocator!");
        }

        // Create the command list
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, CommandAllocator, nullptr, IID_PPV_ARGS(&CommandList));
        
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

    DX12ComputeCommandList::~DX12ComputeCommandList()
    {
        CloseHandle(FenceEvent);
    }

    void DX12ComputeCommandList::Reset()
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

    void DX12ComputeCommandList::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox)
    {
        CommandList->CopyTextureRegion(dst, dstX, dstY, dstZ, src, srcBox);
    }

    void DX12ComputeCommandList::UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
    {
        UpdateSubresources(CommandList, destResource, srcResource, 0, 0, numSubresources, subresourceData);
    }

    void DX12ComputeCommandList::Dispatch(Point3 groupCount)
    {
        CommandList->Dispatch(groupCount.x, groupCount.y, groupCount.z);
    }

    void DX12ComputeCommandList::Execute(ID3D12CommandQueue* commandQueue)
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
    }

    void DX12ComputeCommandList::WaitForCompletion()
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

    void DX12ComputeCommandList::ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier)
    {
        CommandList->ResourceBarrier(count, barrier);
    }

    void DX12ComputeCommandList::Close()
    {
        //close the command list to indicate we're done recording commands.
        HRESULT hr = CommandList->Close();
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to close command list!");
        }
    }
}
