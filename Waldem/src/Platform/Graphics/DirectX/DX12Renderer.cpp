#include "wdpch.h"
#include "DX12Renderer.h"

#include "D3DX12.h"
#include "DX12Buffer.h"
#include "DX12Helper.h"
#include "DX12PixelShader.h"
#include "DX12Texture.h"

namespace Waldem
{
    void DX12Renderer::Initialize(Window* window)
    {
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController))))
        {
            DebugController->EnableDebugLayer();
        }

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugController1))))
        {
            DebugController1->SetEnableGPUBasedValidation(TRUE);
        }

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

        //Break on any D3D12 error
        if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue))))
        {
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);

            D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumSeverities = _countof(severities);
            filter.DenyList.pSeverityList = severities;

            InfoQueue->PushStorageFilter(&filter);
        }

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

        //Create Depth Stencil Buffer
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        h = Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&DSVHeap));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 DSV Heap");
        }
        
        D3D12_RESOURCE_DESC depthBufferDesc = {};
        depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthBufferDesc.Width = window->GetWidth();
        depthBufferDesc.Height = window->GetHeight();
        depthBufferDesc.DepthOrArraySize = 1;
        depthBufferDesc.MipLevels = 1;
        depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthBufferDesc.SampleDesc.Count = 1;
        depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthBufferDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&DepthStencilBuffer)
        );

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        DSVHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
        Device->CreateDepthStencilView(DepthStencilBuffer, &dsvDesc, DSVHandle);

        WorldCommandList.first = new DX12CommandList(Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    void DX12Renderer::Begin()
    {
        auto& cmd = WorldCommandList.first;

        cmd->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = RenderTargets[FrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &barrier);

        CurrentRenderTargetHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
        CurrentRenderTargetHandle.ptr += FrameIndex * RTVDescriptorSize;
        
        if(!WorldCommandList.second)
        {
            cmd->Clear(CurrentRenderTargetHandle, DSVHandle, { 0.0f, 0.0f, 0.0f });
            cmd->Begin(&Viewport, &ScissorRect, CurrentRenderTargetHandle, DSVHandle);
            
            WorldCommandList.second = true;
        }
    }

    void DX12Renderer::End()
    {
        auto& cmd = WorldCommandList.first;

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = RenderTargets[FrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &barrier);
        
        if(WorldCommandList.second)
        {
            cmd->End();
            
            WorldCommandList.second = false;
        }

        cmd->Execute(CommandQueue);
        
        cmd->WaitForCompletion();
    }

    void DX12Renderer::Present()
    {
        HRESULT h = SwapChain->Present(1, 0);

        if(FAILED(h))
        {
            DX12Helper::PrintHResultError(h);
            DX12Helper::PrintDeviceRemovedReason(Device);
        }

        FrameIndex ++;
        FrameIndex %= SWAPCHAIN_SIZE;
    }

    PixelShader* DX12Renderer::LoadShader(std::string shaderName, std::vector<Resource> resources)
    {
        return new DX12PixelShader(shaderName, Device, WorldCommandList.first, resources);
    }

    Texture2D* DX12Renderer::CreateTexture(std::string name, int width, int height, int channels, uint8_t* data)
    {
        Texture2D* texture = new DX12Texture(name, Device, WorldCommandList.first, width, height, channels, data);
        return texture;
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
