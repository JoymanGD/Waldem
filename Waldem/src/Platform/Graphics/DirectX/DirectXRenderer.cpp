#include "wdpch.h"
#include "DirectXRenderer.h"

#include "DX12PixelShader.h"

namespace Waldem
{
    void DirectXRenderer::Initialize(Window* window)
    {
        CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory));

        D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&Device));

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CommandQueue));

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

        DxgiFactory->CreateSwapChain(CommandQueue, &swapChainDesc, &SwapChain);

        //create descriptor Heap for Render Target View (RTV)
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = SWAPCHAIN_SIZE;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RTVHeap));
        RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVHeap->GetCPUDescriptorHandleForHeapStart());
        for (uint32_t i = 0; i < SWAPCHAIN_SIZE; ++i)
        {
            SwapChain->GetBuffer(i, IID_PPV_ARGS(&RenderTargets[i]));
            Device->CreateRenderTargetView(RenderTargets[i], nullptr, rtvHandle);
            rtvHandle.ptr += RTVDescriptorSize;
        }
    }

    void DirectXRenderer::Begin()
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

    void DirectXRenderer::End()
    {
        if(WorldCommandList.second)
        {
            WorldCommandList.first->End();
            
            WorldCommandList.second = false;
        }
    }

    void DirectXRenderer::SetFrameIndex(uint32_t frame)
    {
        FrameIndex = frame;
    }

    PixelShader* DirectXRenderer::LoadShader(std::string shaderName)
    {
        return new DX12PixelShader(Device, shaderName);
    }

    void DirectXRenderer::DrawMesh(Mesh* mesh, PixelShader* pixelShader)
    {
        auto& cmd = WorldCommandList.first;

        cmd->AddDrawCommand(mesh, pixelShader);
    }
}
