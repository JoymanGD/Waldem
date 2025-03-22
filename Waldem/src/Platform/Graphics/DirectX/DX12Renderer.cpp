#include "wdpch.h"
#include "DX12Renderer.h"

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include "D3DX12.h"
#include "DX12AccelerationStructure.h"
#include "DX12Buffer.h"
#include "DX12ComputePipeline.h"
#include "DX12ComputeShader.h"
#include "DX12Helper.h"
#include "DX12GraphicPipeline.h"
#include "DX12PixelShader.h"
#include "DX12RayTracingPipeline.h"
#include "DX12RayTracingShader.h"
#include "DX12RenderTarget.h"
#include "DX12RootSignature.h"
#include "DX12Texture.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdl2.h"

struct ImGuiIO;

namespace Waldem
{
    bool IsDirectXRaytracingSupported(ID3D12Device* testDevice)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport = {};

        if (FAILED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport, sizeof(featureSupport))))
            return false;

        return featureSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    }
    
    void DX12Renderer::Initialize(Window* window)
    {
        CurrentWindow = window;
        
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

        h = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));

        bool rtSupported = IsDirectXRaytracingSupported(Device);

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create DXGIDevice");
        }

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

        //Graphic command queue
        D3D12_COMMAND_QUEUE_DESC graphicQueueDesc = {};
        graphicQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        graphicQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        h = Device->CreateCommandQueue(&graphicQueueDesc, IID_PPV_ARGS(&GraphicCommandQueue));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 graphic command queue");
        }

        //Compute command queue
        D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
        computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

        h = Device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&ComputeCommandQueue));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create D3D12 compute command queue");
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

        h = DxgiFactory->CreateSwapChain(GraphicCommandQueue, &swapChainDesc, &SwapChain);

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
        
        WorldCommandList.first = new DX12CommandList(Device);
        
        Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&WaitFence));
        Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&SecondaryWaitFence));
    }

    void DX12Renderer::InitializeUI()
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ImGuiHeap));
        
        // Platform/Renderer bindings
        ImGui_ImplDX12_Init(Device, SWAPCHAIN_SIZE, DXGI_FORMAT_R8G8B8A8_UNORM, ImGuiHeap, ImGuiHeap->GetCPUDescriptorHandleForHeapStart(), ImGuiHeap->GetGPUDescriptorHandleForHeapStart());
    }

    void DX12Renderer::Draw(Model* model)
    {
        auto& cmd = WorldCommandList.first;

        cmd->Draw(model);
    }

    void DX12Renderer::Draw(CMesh* mesh)
    {
        auto& cmd = WorldCommandList.first;

        cmd->Draw(mesh);
    }

    void DX12Renderer::Signal()
    {
        GraphicCommandQueue->Signal(SecondaryWaitFence, ++SecondaryWaitFenceValue);
    }

    void DX12Renderer::Wait()
    {
        if (SecondaryWaitFence->GetCompletedValue() < SecondaryWaitFenceValue)
        {
            HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            SecondaryWaitFence->SetEventOnCompletion(SecondaryWaitFenceValue, eventHandle);
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
            SecondaryWaitFenceValue--;
        }
    }

    Point3 DX12Renderer::GetNumThreadsPerGroup(ComputeShader* computeShader)
    {
        ID3D12ShaderReflection* shaderReflection;

        IDxcBlob* computeShaderBytecode = (IDxcBlob*)computeShader->GetPlatformData();
        
        Microsoft::WRL::ComPtr<IDxcUtils> dxUtils;
        HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxUtils));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create DXC Utilities.");
        }

        DxcBuffer shaderBuffer = {};
        shaderBuffer.Ptr = computeShaderBytecode->GetBufferPointer();
        shaderBuffer.Size = computeShaderBytecode->GetBufferSize();
        shaderBuffer.Encoding = DXC_CP_ACP;

        hr = dxUtils->CreateReflection(&shaderBuffer, IID_PPV_ARGS(&shaderReflection));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to reflect shader.");
        }

        uint32_t threadGroupX = 0, threadGroupY = 0, threadGroupZ = 0;
        hr = shaderReflection->GetThreadGroupSize(&threadGroupX, &threadGroupY, &threadGroupZ);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to retrieve thread group size.");
        }

        return { threadGroupX, threadGroupY, threadGroupZ };
    }

    void DX12Renderer::Compute(Point3 groupCount)
    {
        WorldCommandList.first->Dispatch(groupCount);
    }

    void DX12Renderer::TraceRays(Pipeline* rayTracingPipeline, Point3 numRays)
    {
        WorldCommandList.first->TraceRays(rayTracingPipeline, numRays);
    }

    void DX12Renderer::Begin()
    {
        auto& worldCmd = WorldCommandList.first;

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = RenderTargets[FrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        worldCmd->ResourceBarrier(1, &barrier);

        CurrentRenderTargetHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
        CurrentRenderTargetHandle.ptr += FrameIndex * RTVDescriptorSize;
        
        if(!WorldCommandList.second)
        {
            worldCmd->Clear(CurrentRenderTargetHandle, DSVHandle, { 0.0f, 0.0f, 0.0f });
            worldCmd->BeginInternal(&Viewport, &ScissorRect, CurrentRenderTargetHandle, DSVHandle);
            
            WorldCommandList.second = true;
        }
    }

    void DX12Renderer::End()
    {
        auto& worldCmd = WorldCommandList.first; 
        
        if(WorldCommandList.second)
        {
            worldCmd->EndInternal();
            
            WorldCommandList.second = false;
        }

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = RenderTargets[FrameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        worldCmd->ResourceBarrier(1, &barrier);
        
        worldCmd->Close();

        worldCmd->Execute(GraphicCommandQueue);

        worldCmd->Reset();
    }

    void DX12Renderer::Present()
    {
        HRESULT h = SwapChain->Present(0, 0);

        if(FAILED(h))
        {
            DX12Helper::PrintHResultError(h);
            DX12Helper::PrintDeviceRemovedReason(Device);
        }

        FrameIndex ++;
        FrameIndex %= SWAPCHAIN_SIZE;
    }

    PixelShader* DX12Renderer::LoadPixelShader(String shaderName, String entryPoint)
    {
        return new DX12PixelShader(shaderName, entryPoint);
    }

    ComputeShader* DX12Renderer::LoadComputeShader(String shaderName, String entryPoint)
    {
        return new DX12ComputeShader(shaderName, entryPoint);
    }

    RayTracingShader* DX12Renderer::LoadRayTracingShader(String shaderName)
    {
        return new DX12RayTracingShader(shaderName);
    }

    void DX12Renderer::SetPipeline(Pipeline* pipeline)
    {
        WorldCommandList.first->SetPipeline(pipeline);
    }

    void DX12Renderer::SetRootSignature(RootSignature* rootSignature)
    {
        WorldCommandList.first->SetRootSignature(rootSignature);
    }

    void DX12Renderer::SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil, SViewport viewport, SScissorRect scissor)
    {
        WorldCommandList.first->SetRenderTargets(renderTargets, depthStencil, viewport, scissor);
    }

    Pipeline* DX12Renderer::CreateGraphicPipeline(const String& name, RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout)
    {
        return new DX12GraphicPipeline(name, Device, rootSignature, shader, RTFormats, rasterizerDesc, depthStencilDesc, primitiveTopologyType, inputLayout);
    }

    Pipeline* DX12Renderer::CreateComputePipeline(const String& name, RootSignature* rootSignature, ComputeShader* shader)
    {
        return new DX12ComputePipeline(name, Device, rootSignature, shader);
    }

    Pipeline* DX12Renderer::CreateRayTracingPipeline(const String& name, RootSignature* rootSignature, RayTracingShader* shader)
    {
        return new DX12RayTracingPipeline(name, Device, rootSignature, shader);
    }

    RootSignature* DX12Renderer::CreateRootSignature(WArray<Resource> resources)
    {
        return new DX12RootSignature(Device, WorldCommandList.first, resources);
    }

    Texture2D* DX12Renderer::CreateTexture(String name, int width, int height, TextureFormat format, uint8_t* data)
    {
        Texture2D* texture = new DX12Texture(name, Device, WorldCommandList.first, width, height, format, data);
        return texture;
    }

    RenderTarget* DX12Renderer::CreateRenderTarget(String name, int width, int height, TextureFormat format)
    {
        DX12RenderTarget* renderTarget = new DX12RenderTarget(name, Device, WorldCommandList.first, width, height, format);
        return renderTarget;
    }

    AccelerationStructure* DX12Renderer::CreateBLAS(String name, WArray<RayTracingGeometry>& geometries)
    {
        return new DX12AccelerationStructure(name, Device, WorldCommandList.first, AccelerationStructureType::BottomLevel, geometries);
    }

    AccelerationStructure* DX12Renderer::CreateTLAS(String name, WArray<RayTracingInstance>& instances)
    {
        return new DX12AccelerationStructure(name, Device, WorldCommandList.first, AccelerationStructureType::TopLevel, instances);
    }

    void DX12Renderer::UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries)
    {
        DX12AccelerationStructure* dx12BLAS = (DX12AccelerationStructure*)BLAS;
        dx12BLAS->Update(WorldCommandList.first, geometries);
    }

    void DX12Renderer::UpdateTLAS(AccelerationStructure* TLAS, WArray<RayTracingInstance>& instances)
    {
        DX12AccelerationStructure* dx12TLAS = (DX12AccelerationStructure*)TLAS;
        dx12TLAS->Update(WorldCommandList.first, instances);
    }

    void DX12Renderer::CopyRenderTarget(RenderTarget* dstRT, RenderTarget* srcRT)
    {
        WorldCommandList.first->CopyRenderTarget(dstRT, srcRT);
    }

    void DX12Renderer::CopyBuffer(Buffer* dstBuffer, Buffer* srcBuffer)
    {
        WorldCommandList.first->CopyBuffer(dstBuffer, srcBuffer);
    }

    Buffer* DX12Renderer::CreateBuffer(String name, BufferType type, void* data, uint32_t size, uint32_t stride)
    {
        return new DX12Buffer(Device, WorldCommandList.first, name, type, data, size, stride);
    }

    void DX12Renderer::UpdateBuffer(Buffer* buffer, void* data, uint32_t size)
    {
        WorldCommandList.first->UpdateBuffer(buffer, data, size);
    }

    void DX12Renderer::ClearRenderTarget(RenderTarget* rt)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dx12RenderTarget = ((DX12RenderTarget*)rt)->GetRenderTargetHandle();
        WorldCommandList.first->ClearRenderTarget(dx12RenderTarget);
    }

    void DX12Renderer::ClearDepthStencil(RenderTarget* ds)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dx12DepthStencil = ((DX12RenderTarget*)ds)->GetRenderTargetHandle();
        WorldCommandList.first->ClearDepthStencil(dx12DepthStencil);
    }

    void DX12Renderer::ResourceBarrier(RenderTarget* rt, ResourceStates before, ResourceStates after)
    {
        ID3D12Resource* resource = (ID3D12Resource*)rt->GetPlatformResource();
        WorldCommandList.first->ResourceBarrier(resource, (D3D12_RESOURCE_STATES)before, (D3D12_RESOURCE_STATES)after);
    }

    void DX12Renderer::ResourceBarrier(Buffer* buffer, ResourceStates before, ResourceStates after)
    {
        ID3D12Resource* resource = (ID3D12Resource*)buffer->GetPlatformResource();
        WorldCommandList.first->ResourceBarrier(resource, (D3D12_RESOURCE_STATES)before, (D3D12_RESOURCE_STATES)after);
    }

    void DX12Renderer::BeginUI()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void DX12Renderer::EndUI()
    {
        auto& worldCmd = WorldCommandList.first;

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        worldCmd->SetDescriptorHeaps(1, &ImGuiHeap);
        ImGui_ImplDX12_RenderDrawData(drawData, (ID3D12GraphicsCommandList*)worldCmd->GetNativeCommandList());
    }
}
