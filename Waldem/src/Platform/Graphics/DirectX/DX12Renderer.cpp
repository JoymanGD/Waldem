#include "wdpch.h"
#include "DX12Renderer.h"

#include "DX12Buffer.h"
#include "DX12PixelShader.h"
#include "DX12Texture.h"

namespace Waldem
{
    void DX12Renderer::Initialize(Window* window)
    {
        Viewport.Width = window->GetWidth();
        Viewport.Height = window->GetHeight();
        
        ScissorRect.right = window->GetWidth();
        ScissorRect.bottom = window->GetHeight();
        
        HRESULT h = CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create DXGIFactory");
        }

        h = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 Device");
        }

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        h = Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 command queue");
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferCount = SWAPCHAIN_SIZE;  // Double buffering
        swapChainDesc.BufferDesc.Width = window->GetWidth();
        swapChainDesc.BufferDesc.Height = window->GetHeight();
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = window->GetWindowsHandle();
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        h = DxgiFactory->CreateSwapChain(CommandQueue, &swapChainDesc, &SwapChain);

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 SwapChain");
        }

        //create descriptor Heap for Render Target View (RTV)
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = SWAPCHAIN_SIZE;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        h = Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RTVHeap));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 RTV Heap");
        }
        
        RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());
        for (uint32_t i = 0; i < SWAPCHAIN_SIZE; ++i)
        {
            h = SwapChain->GetBuffer(i, IID_PPV_ARGS(&RenderTargets[i]));

            if(FAILED(h))
            {
                throw std::runtime_error("Failed to get D3D12 SwapChain buffer");
            }
            
            Device->CreateRenderTargetView(RenderTargets[i], nullptr, rtvHandle);
            rtvHandle.ptr += RTVDescriptorSize;
        }

        WorldCommandList.first = new DX12CommandList(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    void DX12Renderer::Begin()
    {
        auto& cmd = WorldCommandList.first;
        
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = RenderTargets[FrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &barrier);

        CurrentRenderTarget = RTVHeap->GetCPUDescriptorHandleForHeapStart();
        CurrentRenderTarget.ptr += FrameIndex * RTVDescriptorSize;
        
        if(!WorldCommandList.second)
        {
            cmd->Clear(CurrentRenderTarget, { 0.0f, 0.0f, 0.0f });
            cmd->Begin(&Viewport, &ScissorRect, CurrentRenderTarget);
            
            WorldCommandList.second = true;
        }
    }

    void DX12Renderer::End()
    {
        auto& cmd = WorldCommandList.first;
        
        if(WorldCommandList.second)
        {
            cmd->End();
            
            WorldCommandList.second = false;
        }

        cmd->Execute(CommandQueue);
    }

    void DX12Renderer::Present()
    {
        HRESULT h = SwapChain->Present(1, 0);

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to present SwapChain");
        }

        FrameIndex ++;
        FrameIndex %= SWAPCHAIN_SIZE;
    }

    PixelShader* DX12Renderer::LoadShader(std::string shaderName)
    {
        return new DX12PixelShader(Device, shaderName);
    }

    Texture2D* DX12Renderer::CreateTexture(std::string name, int width, int height, int channels, uint8_t* data)
    {
        return new DX12Texture(Device, name, width, height, channels, data);
    }

    VertexBuffer* DX12Renderer::CreateVertexBuffer(void* data, uint32_t size)
    {
        return new DX12VertexBuffer(Device, data, size);
    }

    IndexBuffer* DX12Renderer::CreateIndexBuffer(void* data, uint32_t count)
    {
        return new DX12IndexBuffer(Device, data, count);
    }

    void DX12Renderer::Draw(Mesh* mesh, PixelShader* pixelShader)
    {
        auto& cmd = WorldCommandList.first;

        cmd->AddDrawCommand(mesh, pixelShader);
    }
}
