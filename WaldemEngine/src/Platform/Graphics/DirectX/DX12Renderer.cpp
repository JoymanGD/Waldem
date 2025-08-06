#include "wdpch.h"
#include "DX12Renderer.h"

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include "D3DX12.h"
#include "DX12ComputePipeline.h"
#include "DX12ComputeShader.h"
#include "DX12Helper.h"
#include "DX12GraphicPipeline.h"
#include "DX12PixelShader.h"
#include "DX12RayTracingPipeline.h"
#include "DX12RayTracingShader.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl2.h"
#include "Waldem/Editor/Editor.h"
#include "Waldem/Editor/UIStyles.h"
#include "Waldem/Renderer/Model/Quad.h"

struct ImGuiIO;

using Microsoft::WRL::ComPtr;

namespace Waldem
{
    bool IsDirectXRaytracingSupported(ID3D12Device* testDevice)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport = {};

        if (FAILED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport, sizeof(featureSupport))))
            return false;

        return featureSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    }
    
    void DX12Renderer::Initialize(CWindow* window)
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
        
        HRESULT h = CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create DXGIFactory");
        }

        h = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&Device));

        if(FAILED(h))
        {
            DX12Helper::PrintHResultError(h);
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

        //WorldCommandList
        WorldCommandList.first = new DX12CommandList(Device);
        
        Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&WaitFence));
        Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&SecondaryWaitFence));

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
        rtvHeapDesc.NumDescriptors = RTV_MAX_DESCRIPTORS;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        h = Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RTVHeap));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create RTV descriptor heap");
        }

        //create descriptor Heap for Render Target View (RTV)
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = RTV_MAX_DESCRIPTORS;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        h = Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&DSVHeap));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create RTV descriptor heap");
        }

        InitializeBindless();
        InitializeUI();
    }

    void DX12Renderer::InitializeBindless()
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = BINDLESS_MAX_DESCRIPTORS;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT h = Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&GeneralResourcesHeap));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create General resources descriptor heap");
        }

        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.NumDescriptors = 32;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        h = Device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&GeneralSamplerHeap));

        if(FAILED(h))
        {
            throw std::runtime_error("Failed to create General samplers descriptor heap");
        }

        CreateGeneralRootSignature();
        
        D3D12_INDIRECT_ARGUMENT_DESC args[2] = {};
        args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        args[0].Constant.RootParameterIndex = 0;
        args[0].Constant.Num32BitValuesToSet = 1;
        args[0].Constant.DestOffsetIn32BitValues = 0;
        
        args[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC commandSigDesc = {};
        commandSigDesc.ByteStride = sizeof(IndirectCommand);
        commandSigDesc.NumArgumentDescs = 2;
        commandSigDesc.pArgumentDescs = args;

        Device->CreateCommandSignature(&commandSigDesc, GeneralRootSignature, IID_PPV_ARGS(&GeneralCommandSignature));
    }

    void DX12Renderer::CreateGeneralRootSignature()
    {
        CD3DX12_ROOT_PARAMETER1 rootParameters[2];
        rootParameters[0].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsConstants(32, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

        // Define a static sampler
        D3D12_STATIC_SAMPLER_DESC staticSampler;
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        staticSampler.MipLODBias = 0;
        staticSampler.MaxAnisotropy = 1;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        staticSampler.MinLOD = 0.0f;
        staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        staticSampler.ShaderRegister = 0;
        staticSampler.RegisterSpace = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
        rootSigDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED);

        // === Serialize and Create Root Signature ===
        ComPtr<ID3DBlob> serializedRootSig = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;

        HRESULT hr = D3DX12SerializeVersionedRootSignature(
            &rootSigDesc,
            D3D_ROOT_SIGNATURE_VERSION_1_1,
            &serializedRootSig,
            &errorBlob
        );

        if (FAILED(hr)) {
            if (errorBlob) {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }
            DX12Helper::PrintHResultError(hr);
        }

        hr = Device->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&GeneralRootSignature)
        );

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create root signature.");
        }
    }

    RenderTarget* DX12Renderer::CreateRenderTarget(WString name, int width, int height, TextureFormat format, ID3D12DescriptorHeap* externalHeap, uint slot)
    {
        RenderTarget* renderTarget = new RenderTarget(name, width, height, format);

        uint index = RenderTargetAllocator.Allocate();
        renderTarget->SetIndex(index, RTV_DSV);
        
        UINT descriptorSize;

        HRESULT hr;

        ID3D12Resource* Resource;
        
        D3D12_RESOURCE_DESC textureDesc = {};
        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        if(format == TextureFormat::D32_FLOAT)
        {
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            clearValue.Format = DXGI_FORMAT_D32_FLOAT;
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            
            hr = Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&Resource));

            descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            auto dsvHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
            dsvHandle.ptr += index * descriptorSize;
            //create dsv
            Device->CreateDepthStencilView(Resource, &dsvDesc, dsvHandle);
        }
        else
        {
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = (DXGI_FORMAT)format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            clearValue.Format = (DXGI_FORMAT)format;
            clearValue.Color[0] = 0.0f;
            clearValue.Color[1] = 0.0f;
            clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 1.0f;
            
            hr = Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&Resource));

            descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            auto rtvHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += index * descriptorSize;
            
            //create rtv
            Device->CreateRenderTargetView(Resource, nullptr, rtvHandle);
        }

        //create srv
        renderTarget->SetIndex(slot, SRV_UAV_CBV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = externalHeap->GetCPUDescriptorHandleForHeapStart();
        descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        srvHandle.ptr += slot * descriptorSize;

        auto gpuHandle = externalHeap->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += slot * descriptorSize;
        renderTarget->SetGPUAddress(gpuHandle.ptr);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = (DXGI_FORMAT)format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        Device->CreateShaderResourceView(Resource, &srvDesc, srvHandle);

        ResourceMap[(GraphicResource*)renderTarget] = Resource;

        renderTarget->SetCurrentState((ResourceStates)D3D12_RESOURCE_STATE_RENDER_TARGET);

        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }
        
        Resource->SetName(std::wstring(name.Begin(), name.End()).c_str());

        return renderTarget;
    }

    RenderTarget* DX12Renderer::CreateRenderTarget(WString name, int width, int height, TextureFormat format, ID3D12Resource* resource)
    {
        RenderTarget* renderTarget = new RenderTarget(name, width, height, format);

        uint index = RenderTargetAllocator.Allocate();
        renderTarget->SetIndex(index, RTV_DSV);
        
        UINT descriptorSize;

        HRESULT hr;

        D3D12_RESOURCE_DESC textureDesc = {};
        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        if(format == TextureFormat::D32_FLOAT)
        {
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            clearValue.Format = DXGI_FORMAT_D32_FLOAT;
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            auto dsvHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
            dsvHandle.ptr += index * descriptorSize;
            
            //create dsv
            Device->CreateDepthStencilView(resource, &dsvDesc, dsvHandle);

            format = TextureFormat::R32_FLOAT;
        }
        else
        {
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = (DXGI_FORMAT)format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            clearValue.Format = (DXGI_FORMAT)format;
            clearValue.Color[0] = 0.0f;
            clearValue.Color[1] = 0.0f;
            clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 1.0f;

            descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            auto rtvHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += index * descriptorSize;
            
            //create rtv
            Device->CreateRenderTargetView(resource, nullptr, rtvHandle);
        }

        //create srv
        index = GeneralAllocator.Allocate();
        renderTarget->SetIndex(index, SRV_UAV_CBV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = GeneralResourcesHeap->GetCPUDescriptorHandleForHeapStart();
        descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        srvHandle.ptr += index * descriptorSize;

        renderTarget->SetGPUAddress(srvHandle.ptr);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = (DXGI_FORMAT)format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        Device->CreateShaderResourceView(resource, &srvDesc, srvHandle);

        ResourceMap[(GraphicResource*)renderTarget] = resource;

        renderTarget->SetCurrentState((ResourceStates)D3D12_RESOURCE_STATE_RENDER_TARGET);
        
        resource->SetName(std::wstring(name.Begin(), name.End()).c_str());

        return renderTarget;
    }

    void DX12Renderer::SetRenderTargets()
    {
        WArray<D3D12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
        D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle = nullptr;

        WArray<RenderTarget*>& renderTargets = DefaultRenderPassState.RenderTargets;
        RenderTarget* depthStencil = DefaultRenderPassState.DepthStencil;
        
        if(CurrentRenderPassState.RenderTargetsDirty && CurrentRenderPassState.RenderTargets.Num() > 0)
        {
            renderTargets = CurrentRenderPassState.RenderTargets;
            CurrentRenderPassState.RenderTargetsDirty = false;
        }

        if(CurrentRenderPassState.DepthStencilDirty)
        {
            depthStencil = CurrentRenderPassState.DepthStencil;
            CurrentRenderPassState.DepthStencilDirty = false;
        }
        
        for (auto rt : renderTargets)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtv = RTVHeap->GetCPUDescriptorHandleForHeapStart();
            rtv.ptr += rt->GetIndex(RTV_DSV) * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            rtvHandles.Add(rtv);
        }
        
        if(depthStencil)
        {
            auto cpuHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
            cpuHandle.ptr += depthStencil->GetIndex(RTV_DSV) * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            dsvHandle = &cpuHandle;
        }
        
        WorldCommandList.first->SetRenderTargets(rtvHandles, dsvHandle);
    }

    void DX12Renderer::InitializeUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable multi-viewport

        SDL_Window* window = static_cast<SDL_Window*>(CWindow::GetNativeWindow());
        ImGui_ImplSDL2_InitForD3D(window);
        
        Vector2 size = Vector2(CurrentWindow->GetWidth(), CurrentWindow->GetHeight());
        
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 3;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ImGuiHeap));

        WArray editorRenderTargets = { CreateRenderTarget("EditorViewRT", size.x, size.y, TextureFormat::R8G8B8A8_UNORM, ImGuiHeap, 1) };
        RenderTarget* editorViewportDepth = CreateRenderTarget("EditorViewDepth", size.x, size.y, TextureFormat::D32_FLOAT);
        auto editorFrameBuffer = new SFrameBuffer(editorRenderTargets, editorViewportDepth);
        EditorViewport = SViewport(Vector2(0, 0), Vector2(size.x, size.y), Vector2(0, 1), editorFrameBuffer);

        EditorViewport.SubscribeOnResize([this](Point2 size)
        {
            if (size.x > 0 && size.y > 0)
            {
                ResizeTriggered = true;
                NewSize = size;
            }
        });
        
        WArray gameRenderTargets = { CreateRenderTarget("GameViewRT", size.x, size.y, TextureFormat::R8G8B8A8_UNORM, ImGuiHeap, 2) };
        RenderTarget* gameViewportDepth = CreateRenderTarget("GameViewDepth", size.x, size.y, TextureFormat::D32_FLOAT);
        auto gameFrameBuffer = new SFrameBuffer(gameRenderTargets, gameViewportDepth);
        GameViewport = SViewport(Vector2(0, 0), Vector2(size.x, size.y), Vector2(0, 1), gameFrameBuffer);
        
        // Platform/Renderer bindings
        ImGui_ImplDX12_Init(Device, SWAPCHAIN_SIZE, DXGI_FORMAT_R8G8B8A8_UNORM, ImGuiHeap, ImGuiHeap->GetCPUDescriptorHandleForHeapStart(), ImGuiHeap->GetGPUDescriptorHandleForHeapStart());

        UIStyles::ApplyDefault();

        //create main framebuffer and viewport
        auto mainFrameBuffer = new SFrameBuffer();
        for (uint32_t i = 0; i < SWAPCHAIN_SIZE; ++i)
        {
            ID3D12Resource* resource;
            HRESULT h = SwapChain->GetBuffer(i, IID_PPV_ARGS(&resource));

            auto renderTarget = CreateRenderTarget("MainViewport_FrameBuffer_" + std::to_string(i), size.x, size.y, TextureFormat::R8G8B8A8_UNORM, resource);
            ResourceBarrier(renderTarget, ALL_SHADER_RESOURCE, PRESENT);

            if(FAILED(h))
            {
                throw std::runtime_error("Failed to get D3D12 SwapChain buffer");
            }

            mainFrameBuffer->AddRenderTarget(renderTarget);
        }
        mainFrameBuffer->SetDepth(CreateRenderTarget("MainViewport_Depth", size.x, size.y, TextureFormat::D32_FLOAT));
        
        MainViewport = SViewport(Point2(0, 0), Point2(size.x, size.y), Point2(0, 1), mainFrameBuffer);

        MainViewport.SubscribeOnResize([this](Point2 size)
        {
            if (size.x > 0 && size.y > 0)
            {
                Wait();
                
                // Release old RTVs
                MainViewport.FrameBuffer->Destroy();

                // Resize swapchain buffers
                DXGI_SWAP_CHAIN_DESC desc = {};
                SwapChain->GetDesc(&desc);
                HRESULT h = SwapChain->ResizeBuffers(SWAPCHAIN_SIZE, size.x, size.y, desc.BufferDesc.Format, desc.Flags);
                if (FAILED(h))
                {
                    throw std::runtime_error("Failed to resize DX12 swapchain buffers.");
                }
                
                for (uint32_t i = 0; i < SWAPCHAIN_SIZE; ++i)
                {
                    ID3D12Resource* resource;
                    h = SwapChain->GetBuffer(i, IID_PPV_ARGS(&resource));

                    auto renderTarget = CreateRenderTarget("MainViewport_FrameBuffer_" + std::to_string(i), size.x, size.y, TextureFormat::R8G8B8A8_UNORM, resource);
                    ResourceBarrier(renderTarget, ALL_SHADER_RESOURCE, PRESENT);

                    if(FAILED(h))
                    {
                        throw std::runtime_error("Failed to get D3D12 SwapChain buffer");
                    }

                    MainViewport.FrameBuffer->AddRenderTarget(renderTarget);
                }

                MainViewport.FrameBuffer->SetDepth(CreateRenderTarget("MainViewport_Depth", size.x, size.y, TextureFormat::D32_FLOAT));
            }
        });
    }

    void DX12Renderer::DeinitializeUI()
    {
        ImGui_ImplSDL2_Shutdown();
        ImGui_ImplDX12_Shutdown();
        ImGui::DestroyContext();
    }

    void DX12Renderer::ResizeEditorViewport(Vector2 size)
    {
        // Release old RTVs
        EditorViewport.FrameBuffer->Destroy();

        RenderTarget* editorRenderTarget = { CreateRenderTarget("EditorViewRT", size.x, size.y, TextureFormat::R8G8B8A8_UNORM, ImGuiHeap, 1) };
        EditorViewport.FrameBuffer->AddRenderTarget(editorRenderTarget);

        RenderTarget* editorViewportDepth = CreateRenderTarget("EditorViewDepth", size.x, size.y, TextureFormat::D32_FLOAT);
        EditorViewport.FrameBuffer->SetDepth(editorViewportDepth);
    }

    void DX12Renderer::Draw(CMesh* mesh)
    {
        SetRenderTargets();
        
        auto& cmd = WorldCommandList.first;

        cmd->Draw(mesh);
    }

    void DX12Renderer::DrawIndirect(uint numCommands, Buffer* indirectBuffer)
    {
        SetRenderTargets();
        
        auto& cmd = WorldCommandList.first;
        ID3D12Resource* resource = ResourceMap[(GraphicResource*)indirectBuffer];
        cmd->DrawIndirect(GeneralCommandSignature, numCommands, resource);
    }

    void DX12Renderer::SetIndexBuffer(Buffer* indexBuffer)
    {
        auto& cmd = WorldCommandList.first;
        cmd->SetIndexBuffer(indexBuffer);
    }

    void DX12Renderer::SetVertexBuffers(Buffer* vertexBuffer, uint32 numBuffers, uint32 startIndex)
    {
        auto& cmd = WorldCommandList.first;
        cmd->SetVertexBuffers(vertexBuffer, numBuffers, startIndex);
    }

    void DX12Renderer::Signal()
    {
        GraphicCommandQueue->Signal(SecondaryWaitFence, ++SecondaryWaitFenceValue);
    }

    void DX12Renderer::Wait()
    {
        Signal();
        
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

        shaderReflection->Release();

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
        auto color = EditorViewport.FrameBuffer->GetCurrentRenderTarget();
        auto depth = EditorViewport.FrameBuffer->GetDepth();

        DefaultRenderPassState.RenderTargets = { color };
        DefaultRenderPassState.DepthStencil = depth;
        DefaultRenderPassState.RenderTargetsDirty = true;
        DefaultRenderPassState.DepthStencilDirty = true;

        ResourceBarrier(color, ALL_SHADER_RESOURCE, RENDER_TARGET);
        ResourceBarrier(depth, ALL_SHADER_RESOURCE, DEPTH_WRITE);
        
        WorldCommandList.first->SetGeneralDescriptorHeaps(GeneralResourcesHeap, GeneralSamplerHeap);
        WorldCommandList.first->SetRootSignature(GeneralRootSignature);
        uint data = 999;
        WorldCommandList.first->SetConstants(0, 1, &data, PipelineType::Graphics);
        
        if(!WorldCommandList.second)
        {
            ClearRenderTarget(color);
            ClearDepthStencil(depth);
            
            SetViewport(EditorViewport);
            
            // worldCmd->BeginInternal(EditorViewport, RTVHeap, DSVHeap);
            
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

        ResourceBarrier(EditorViewport.FrameBuffer->GetCurrentRenderTarget(), RENDER_TARGET, ALL_SHADER_RESOURCE);
        ResourceBarrier(EditorViewport.FrameBuffer->GetDepth(), DEPTH_WRITE, ALL_SHADER_RESOURCE);
        
        worldCmd->Close();

        worldCmd->Execute(GraphicCommandQueue);

        worldCmd->Reset();
    }

    void DX12Renderer::BeginUI()
    {
        auto color = MainViewport.FrameBuffer->GetCurrentRenderTarget();
        auto depth = MainViewport.FrameBuffer->GetDepth();
        
        ResourceBarrier(color, PRESENT, RENDER_TARGET);
        ResourceBarrier(depth, ALL_SHADER_RESOURCE, DEPTH_WRITE);
        
        if(!WorldCommandList.second)
        {
            ClearRenderTarget(color);
            ClearDepthStencil(depth);
            
            SetViewport(MainViewport);
            BindRenderTargets({color});
            BindDepthStencil(depth);
            // worldCmd->BeginInternal(MainViewport, RTVHeap, DSVHeap);
            
            WorldCommandList.second = true;
        }

        SetRenderTargets();
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void DX12Renderer::EndUI()
    {
        auto& worldCmd = WorldCommandList.first;

        if(ResizeTriggered)
        {
            ResizeEditorViewport(NewSize);
            ResizeTriggered = false;
        }

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        worldCmd->SetDescriptorHeaps(1, &ImGuiHeap);
        ImGui_ImplDX12_RenderDrawData(drawData, (ID3D12GraphicsCommandList*)worldCmd->GetNativeCommandList());

        ImGuiIO& io = ImGui::GetIO();
        
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
        
        if(WorldCommandList.second)
        {
            worldCmd->EndInternal();
            
            WorldCommandList.second = false;
        }

        ResourceBarrier(MainViewport.FrameBuffer->GetCurrentRenderTarget(), RENDER_TARGET, PRESENT);
        ResourceBarrier(MainViewport.FrameBuffer->GetDepth(), DEPTH_WRITE, ALL_SHADER_RESOURCE);
        
        worldCmd->Close();

        worldCmd->Execute(GraphicCommandQueue);

        worldCmd->Reset();
    }

    void DX12Renderer::Destroy(GraphicResource* resource)
    {
        if(resource)
        {
            auto dx12Resource = ResourceMap[resource];
            if(dx12Resource)
            {
                ResourceMap.Remove(resource);
                ResourcesToDestroy.Add(dx12Resource);
            }
            else
            {
                WD_CORE_ERROR("Resource not found in ResourceMap");
            }
        }
        else
        {
            WD_CORE_ERROR("Invalid resource");
        }

        auto uploadResource = resource->GetUploadResource();
        if(uploadResource)
        {
            auto dx12UploadResource = ResourceMap[uploadResource];
            if(dx12UploadResource)
            {
                ResourceMap.Remove(uploadResource);
                ResourcesToDestroy.Add(dx12UploadResource);
            }
            else
            {
                WD_CORE_ERROR("Upload resource not found in ResourceMap");
            }
        }
        else
        {
            WD_CORE_ERROR("Resource doesnt have Upload resource");
        }

        auto readbackResource = resource->GetReadbackResource();
        if(readbackResource)
        {
            auto dx12ReadbackResource = ResourceMap[readbackResource];
            if(dx12ReadbackResource)
            {
                ResourceMap.Remove(readbackResource);
                ResourcesToDestroy.Add(dx12ReadbackResource);
            }
            else
            {
                WD_CORE_ERROR("Readback resource not found in ResourceMap");
            }
        }
        else
        {
            WD_CORE_ERROR("Resource doesnt have Readback resource");
        }

        if(resource->GetType() == RTYPE_Texture || resource->GetType() == RTYPE_Buffer || resource->GetType() == RTYPE_AccelerationStructure)
        {
            GeneralAllocator.Free(resource->GetIndex(SRV_UAV_CBV));
        }
        else if(resource->GetType() == RTYPE_RenderTarget)
        {
            RenderTargetAllocator.Free(resource->GetIndex(RTV_DSV));
            GeneralAllocator.Free(resource->GetIndex(SRV_UAV_CBV));
        }
        else
        {
            WD_CORE_ERROR("Unknown resource type");
        }

        for (auto resource : ResourcesToDestroy)
        {
            resource->Release();
        }
        ResourcesToDestroy.Clear();
    }

    void* DX12Renderer::GetPlatformResource(GraphicResource* resource)
    {
        return ResourceMap[resource];
    }

    void DX12Renderer::Present()
    {
        HRESULT h = SwapChain->Present(0, 0);

        if(FAILED(h))
        {
            DX12Helper::PrintHResultError(h);
            DX12Helper::PrintDeviceRemovedReason(Device);
        }

        MainViewport.FrameBuffer->Advance();
    }

    PixelShader* DX12Renderer::LoadPixelShader(const Path& shaderName, WString entryPoint)
    {
        return new DX12PixelShader(shaderName, entryPoint);
    }

    ComputeShader* DX12Renderer::LoadComputeShader(const Path& shaderName, WString entryPoint)
    {
        return new DX12ComputeShader(shaderName, entryPoint);
    }

    RayTracingShader* DX12Renderer::LoadRayTracingShader(const Path& shaderName)
    {
        return new DX12RayTracingShader(shaderName);
    }

    void DX12Renderer::SetPipeline(Pipeline* pipeline)
    {
        WorldCommandList.first->SetPipeline(pipeline);
        CurrentPipelineType = pipeline->CurrentPipelineType;
    }

    void DX12Renderer::PushConstants(void* data, size_t size)
    {
        if(size > 128)
        {
            WD_CORE_ERROR("PushConstants size exceeds 128 bytes");
            return;
        }

        uint num = size / sizeof(uint);
        
        WorldCommandList.first->SetConstants(1, num, data, CurrentPipelineType);
    }

    void DX12Renderer::BindRenderTargets(WArray<RenderTarget*> renderTargets)
    {
        CurrentRenderPassState.RenderTargets = renderTargets;
        CurrentRenderPassState.RenderTargetsDirty = true;
    }

    void DX12Renderer::BindDepthStencil(RenderTarget* depthStencil)
    {
        CurrentRenderPassState.DepthStencil = depthStencil;
        CurrentRenderPassState.DepthStencilDirty = true;
    }

    void DX12Renderer::SetViewport(SViewport& viewport)
    {
        D3D12_VIEWPORT d3d12Viewport = { 0, 0, (float)viewport.Size.x, (float)viewport.Size.y, (float)viewport.DepthRange.x, (float)viewport.DepthRange.y };
        D3D12_RECT d3d12ScissorRect = { 0, 0, viewport.Size.x, viewport.Size.y };

        WorldCommandList.first->SetViewport(d3d12Viewport, d3d12ScissorRect);
    }

    Pipeline* DX12Renderer::CreateGraphicPipeline(const WString& name, PixelShader* shader, WArray<TextureFormat> RTFormats, TextureFormat depthFormat, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout)
    {
        return new DX12GraphicPipeline(name, Device, GeneralRootSignature, shader, RTFormats, depthFormat, rasterizerDesc, depthStencilDesc, primitiveTopologyType, inputLayout);
    }

    Pipeline* DX12Renderer::CreateComputePipeline(const WString& name, ComputeShader* shader)
    {
        return new DX12ComputePipeline(name, Device, GeneralRootSignature, shader);
    }

    Pipeline* DX12Renderer::CreateRayTracingPipeline(const WString& name, RayTracingShader* shader)
    {
        return new DX12RayTracingPipeline(name, Device, GeneralRootSignature, shader);
    }

    Texture2D* DX12Renderer::CreateTexture(WString name, int width, int height, TextureFormat format, uint8_t* data)
    {
        auto cmdList = WorldCommandList.first;
        
        Texture2D* texture = new Texture2D(name, width, height, format, data);
        
        HRESULT hr;
        
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = (DXGI_FORMAT)format;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Create the texture resource
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        ID3D12Resource* Resource;
        
        hr = Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&Resource));

        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }

        std::wstring widestr = std::wstring(name.Begin(), name.End());
        Resource->SetName(widestr.c_str());

        if(data)
        {
            D3D12_HEAP_PROPERTIES uploadHeapProps = {};
            uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            uploadHeapProps.CreationNodeMask = 1;
            uploadHeapProps.VisibleNodeMask = 1;
            
            UINT64 uploadBufferSize;
            Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

            D3D12_RESOURCE_DESC uploadBufferDesc = {};
            uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            uploadBufferDesc.Width = uploadBufferSize;
            uploadBufferDesc.Height = 1;
            uploadBufferDesc.DepthOrArraySize = 1;
            uploadBufferDesc.MipLevels = 1;
            uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
            uploadBufferDesc.SampleDesc.Count = 1;
            uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            
            ID3D12Resource* textureUploadHeap;
            hr = Device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap));
            
            if(FAILED(hr))
            {
                DX12Helper::PrintHResultError(hr);
            }

            D3D12_SUBRESOURCE_DATA subResourceData;
            subResourceData.pData = data;
            subResourceData.RowPitch = uploadBufferSize / height;
            subResourceData.SlicePitch = uploadBufferSize;

            cmdList->UpdateSubresoures(Resource, textureUploadHeap, 1, &subResourceData);
        }

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = Resource;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        texture->SetCurrentState((ResourceStates)D3D12_RESOURCE_STATE_COMMON);

        ResourceMap[(GraphicResource*)texture] = Resource;
        
        uint index = GeneralAllocator.Allocate();
        texture->SetIndex(index, SRV_UAV_CBV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GeneralResourcesHeap->GetCPUDescriptorHandleForHeapStart();
        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpuHandle.ptr += index * descriptorSize;

        texture->SetGPUAddress(cpuHandle.ptr);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = (DXGI_FORMAT)format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        Device->CreateShaderResourceView(Resource, &srvDesc, cpuHandle);
        
        return texture;
    }

    RenderTarget* DX12Renderer::CreateRenderTarget(WString name, int width, int height, TextureFormat format)
    {
        RenderTarget* renderTarget = new RenderTarget(name, width, height, format);

        InitializeRenderTarget(name, width, height, format, renderTarget);

        return renderTarget;
    }

    void DX12Renderer::InitializeRenderTarget(WString name, int width, int height, TextureFormat format, RenderTarget*& renderTarget)
    {
        uint index = RenderTargetAllocator.Allocate();
        renderTarget->SetIndex(index, RTV_DSV);
        
        UINT descriptorSize;

        HRESULT hr;

        ID3D12Resource* Resource;
        
        D3D12_RESOURCE_DESC textureDesc = {};
        D3D12_CLEAR_VALUE clearValue = {};
        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        if(format == TextureFormat::D32_FLOAT)
        {
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            clearValue.Format = DXGI_FORMAT_D32_FLOAT;
            clearValue.DepthStencil.Depth = 1.0f;
            clearValue.DepthStencil.Stencil = 0;

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
            
            hr = Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&Resource));

            descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            auto dsvHandle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
            dsvHandle.ptr += index * descriptorSize;
            
            //create dsv
            Device->CreateDepthStencilView(Resource, &dsvDesc, dsvHandle);

            format = TextureFormat::R32_FLOAT;
        }
        else
        {
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            textureDesc.Width = width;
            textureDesc.Height = height;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.MipLevels = 1;
            textureDesc.Format = (DXGI_FORMAT)format;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

            clearValue.Format = (DXGI_FORMAT)format;
            clearValue.Color[0] = 0.0f;
            clearValue.Color[1] = 0.0f;
            clearValue.Color[2] = 0.0f;
            clearValue.Color[3] = 1.0f;
            
            hr = Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                &clearValue,
                IID_PPV_ARGS(&Resource));

            descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            auto rtvHandle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += index * descriptorSize;
            
            //create rtv
            Device->CreateRenderTargetView(Resource, nullptr, rtvHandle);
        }

        //create srv
        index = GeneralAllocator.Allocate();
        renderTarget->SetIndex(index, SRV_UAV_CBV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = GeneralResourcesHeap->GetCPUDescriptorHandleForHeapStart();
        descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        srvHandle.ptr += index * descriptorSize;

        renderTarget->SetGPUAddress(srvHandle.ptr);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = (DXGI_FORMAT)format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        Device->CreateShaderResourceView(Resource, &srvDesc, srvHandle);

        ResourceMap[(GraphicResource*)renderTarget] = Resource;

        renderTarget->SetCurrentState((ResourceStates)D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }
        
        Resource->SetName(std::wstring(name.Begin(), name.End()).c_str());
    }

    AccelerationStructure* DX12Renderer::CreateBLAS(WString name, WArray<RayTracingGeometry>& geometries)
    {
        auto cmdList = WorldCommandList.first;

        AccelerationStructure* blas = new AccelerationStructure(name, AccelerationStructureType::BottomLevel);
        
        WArray<D3D12_RAYTRACING_GEOMETRY_DESC> dx12Geometries;

        for (auto& geometry : geometries)
        {
            D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
            geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
            
            geomDesc.Triangles.VertexBuffer.StartAddress = geometry.VertexBuffer->GetGPUAddress();
            geomDesc.Triangles.VertexBuffer.StrideInBytes = geometry.VertexBuffer->GetStride();
            geomDesc.Triangles.VertexCount = geometry.VertexBuffer->GetCount();
            geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geomDesc.Triangles.IndexBuffer = geometry.IndexBuffer->GetGPUAddress();
            geomDesc.Triangles.IndexCount = geometry.IndexBuffer->GetCount();
            geomDesc.Triangles.IndexFormat = geometry.IndexBuffer->GetStride() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
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
        Device->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &prebuildInfo);

        prebuildInfo.ScratchDataSizeInBytes = (prebuildInfo.ScratchDataSizeInBytes + 255) & ~255;
        prebuildInfo.ResultDataMaxSizeInBytes = (prebuildInfo.ResultDataMaxSizeInBytes + 255) & ~255;

        //create scratch buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = prebuildInfo.ScratchDataSizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ID3D12Resource* ScratchBuffer;
        Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&ScratchBuffer));

        Buffer* scratchBuffer = new Buffer();
        scratchBuffer->SetGPUAddress(ScratchBuffer->GetGPUVirtualAddress());
        blas->SetScratchBuffer(scratchBuffer);

        //create resource
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ID3D12Resource* Resource;
        Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&Resource));

        blas->SetCurrentState((ResourceStates)D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        std::wstring widestr = std::wstring(name.Begin(), name.End());
        Resource->SetName(widestr.c_str());
        
        ResourceMap[(GraphicResource*)blas] = Resource;

        blas->SetGPUAddress(Resource->GetGPUVirtualAddress());
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = ScratchBuffer->GetGPUVirtualAddress();
        asDesc.DestAccelerationStructureData = Resource->GetGPUVirtualAddress();

        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        cmdList->UAVBarrier(Resource);

        return blas;
    }

    AccelerationStructure* DX12Renderer::CreateTLAS(WString name, Buffer* instanceBuffer, uint numInstances)
    {
        AccelerationStructure* tlas = new AccelerationStructure(name, AccelerationStructureType::TopLevel);

        InitializeTLAS(name, instanceBuffer, numInstances, tlas);

        return tlas;
    }

    void DX12Renderer::InitializeTLAS(WString name, Buffer* instanceBuffer, uint numInstances, AccelerationStructure*& tlas)
    {
        auto cmdList = WorldCommandList.first;
        
        uint index = GeneralAllocator.Allocate();
        tlas->SetIndex(index, SRV_UAV_CBV);

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
        asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        asInputs.NumDescs = numInstances;
        asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
        asInputs.InstanceDescs = instanceBuffer->GetUploadResource()->GetGPUAddress();

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
        Device->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &prebuildInfo);

        prebuildInfo.ScratchDataSizeInBytes = (prebuildInfo.ScratchDataSizeInBytes + 255) & ~255;
        prebuildInfo.ResultDataMaxSizeInBytes = (prebuildInfo.ResultDataMaxSizeInBytes + 255) & ~255;

        //create scratch buffer
        
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = prebuildInfo.ScratchDataSizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ID3D12Resource* ScratchResource;
        Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&ScratchResource));

        Buffer* scratchBuffer = new Buffer(name + "_ScratchBuffer", BufferType::StorageBuffer, prebuildInfo.ScratchDataSizeInBytes, sizeof(int));
        scratchBuffer->SetGPUAddress(ScratchResource->GetGPUVirtualAddress());
        tlas->SetScratchBuffer(scratchBuffer);

        //create resource
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        ID3D12Resource* Resource;
        Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&Resource));

        tlas->SetCurrentState((ResourceStates)D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

        std::wstring widestr = std::wstring(name.Begin(), name.End());
        Resource->SetName(widestr.c_str());
        
        ResourceMap[(GraphicResource*)tlas] = Resource;
        tlas->SetGPUAddress(Resource->GetGPUVirtualAddress());
        
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = ScratchResource->GetGPUVirtualAddress();
        asDesc.DestAccelerationStructureData = Resource->GetGPUVirtualAddress();

        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        cmdList->UAVBarrier(Resource);
        
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GeneralResourcesHeap->GetCPUDescriptorHandleForHeapStart();
        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpuHandle.ptr += index * descriptorSize;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.RaytracingAccelerationStructure.Location = asDesc.DestAccelerationStructureData;

        Device->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);
    }

    void DX12Renderer::UpdateBLAS(AccelerationStructure* BLAS, WArray<RayTracingGeometry>& geometries)
    {
        auto cmdList = WorldCommandList.first;
        
        // Update geometry descriptions
        WArray<D3D12_RAYTRACING_GEOMETRY_DESC> dx12Geometries;
        for (auto& geometry : geometries)
        {
            D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
            geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            geomDesc.Triangles.VertexBuffer.StartAddress = geometry.VertexBuffer->GetGPUAddress();
            geomDesc.Triangles.VertexBuffer.StrideInBytes = geometry.VertexBuffer->GetStride();
            geomDesc.Triangles.VertexCount = geometry.VertexBuffer->GetCount();
            geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            geomDesc.Triangles.IndexBuffer = geometry.IndexBuffer->GetGPUAddress();
            geomDesc.Triangles.IndexCount = geometry.IndexBuffer->GetCount();
            geomDesc.Triangles.IndexFormat = geometry.IndexBuffer->GetStride() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
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
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc;
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = BLAS->GetScratchBuffer()->GetGPUAddress();
        asDesc.DestAccelerationStructureData = BLAS->GetGPUAddress();
        asDesc.SourceAccelerationStructureData = BLAS->GetGPUAddress();

        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        cmdList->UAVBarrier(ResourceMap[(GraphicResource*)BLAS]);
    }

    void DX12Renderer::UpdateTLAS(AccelerationStructure* TLAS, Buffer* instanceBuffer, uint numInstances)
    {
        auto cmdList = WorldCommandList.first;

        // Prepare the acceleration structure inputs for update
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
        asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        asInputs.NumDescs = numInstances;
        asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE |
                         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;  // Important for refit
        asInputs.InstanceDescs = instanceBuffer->GetUploadResource()->GetGPUAddress();

        // Describe the acceleration structure build for update
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
        asDesc.Inputs = asInputs;
        asDesc.ScratchAccelerationStructureData = TLAS->GetScratchBuffer()->GetGPUAddress();
        asDesc.DestAccelerationStructureData = TLAS->GetGPUAddress();
        asDesc.SourceAccelerationStructureData = TLAS->GetGPUAddress();  // Source is the existing TLAS

        // Build the updated TLAS
        cmdList->BuildRaytracingAccelerationStructure(&asDesc);

        // Insert a barrier to ensure completion
        cmdList->UAVBarrier(ResourceMap[(GraphicResource*)TLAS]);
    }

    Buffer* DX12Renderer::CreateBuffer(WString name, BufferType type, uint32_t size, uint32_t stride, void* data, size_t dataSize)
    {
        Buffer* buffer = new Buffer(name, type, size, stride);

        InitializeBuffer(name, type, size, stride, buffer, data, dataSize);

        return buffer;
    }

    void DX12Renderer::InitializeBuffer(WString name, BufferType type, uint32_t size, uint32_t stride, Buffer*& buffer, void* data, size_t dataSize)
    {
        auto cmdList = WorldCommandList.first;
        
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = size;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        ID3D12Resource* Resource;
        
        HRESULT hr = Device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&Resource));

        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create buffer!");
        }

        buffer->SetGPUAddress(Resource->GetGPUVirtualAddress());
        ResourceMap[(GraphicResource*)buffer] = Resource;
        Resource->SetName(std::wstring(name.Begin(), name.End()).c_str());
        
        D3D12_HEAP_PROPERTIES uploadHeapProps;
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadHeapProps.CreationNodeMask = 1;
        uploadHeapProps.VisibleNodeMask = 1;
            
        UINT64 uploadBufferSize;
        Device->GetCopyableFootprints(&bufferDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

        D3D12_RESOURCE_DESC uploadBufferDesc = {};
        uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadBufferDesc.Width = uploadBufferSize;
        uploadBufferDesc.Height = 1;
        uploadBufferDesc.DepthOrArraySize = 1;
        uploadBufferDesc.MipLevels = 1;
        uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadBufferDesc.SampleDesc.Count = 1;
        uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ID3D12Resource* UploadResource;
        hr = Device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadResource));
        WString uploadResourceName = name + " Upload Resource";
        UploadResource->SetName(std::wstring(uploadResourceName.Begin(), uploadResourceName.End()).c_str());

        if(FAILED(hr))
        {
            DX12Helper::PrintHResultError(hr);
        }
        
        auto uploadGraphicResource = new GraphicResource();
        uploadGraphicResource->SetGPUAddress(UploadResource->GetGPUVirtualAddress());
        ResourceMap[uploadGraphicResource] = UploadResource;
        
        buffer->SetUploadResource(uploadGraphicResource);

        if(data && dataSize == 0)
        {
            dataSize = size;
        }

        if(dataSize > 0)
        {
            cmdList->UpdateRes(Resource, UploadResource, data, dataSize, D3D12_RESOURCE_STATE_COMMON, 0);
            buffer->AddSize(dataSize);
        }

        D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_COPY_DEST;
        D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        
        switch (type)
        {
        case VertexBuffer:
            {
                afterState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
                break;
            }
        case IndexBuffer:
            {
                afterState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
                break;
            }
        case IndirectBuffer:
            {                
                afterState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
                break;
            }
        }
        
        cmdList->ResourceBarrier(Resource, beforeState, afterState | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        buffer->SetCurrentState((ResourceStates)(afterState | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
        
        uint index = GeneralAllocator.Allocate();
        buffer->SetIndex(index, SRV_UAV_CBV);
        
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GeneralResourcesHeap->GetCPUDescriptorHandleForHeapStart();
        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpuHandle.ptr += index * descriptorSize;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = buffer->GetCapacity() / buffer->GetStride();
        srvDesc.Buffer.StructureByteStride = buffer->GetStride();
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        Device->CreateShaderResourceView(Resource, &srvDesc, cpuHandle);

        ID3D12Resource* readbackBuffer;
        auto heapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

        D3D12_RESOURCE_DESC readbackBufferDesc = {};
        readbackBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        readbackBufferDesc.Width = size;
        readbackBufferDesc.Height = 1;
        readbackBufferDesc.DepthOrArraySize = 1;
        readbackBufferDesc.MipLevels = 1;
        readbackBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        readbackBufferDesc.SampleDesc.Count = 1;
        readbackBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        readbackBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        
        Device->CreateCommittedResource(
            &heapDesc,
            D3D12_HEAP_FLAG_NONE,
            &readbackBufferDesc, // must match GPU buffer size
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&readbackBuffer));
        
        WString readbackResourceName = name + " Readback Resource";
        readbackBuffer->SetName(std::wstring(readbackResourceName.Begin(), readbackResourceName.End()).c_str());
        
        auto readbackGraphicResource = new GraphicResource();
        ResourceMap[readbackGraphicResource] = readbackBuffer;
        
        buffer->SetReadbackResource(readbackGraphicResource);
    }

    void DX12Renderer::CopyResource(GraphicResource* dstResource, GraphicResource* srcResource)
    {
        ID3D12Resource* dx12DstResource = ResourceMap[dstResource];
        ID3D12Resource* dx12SrcResource = ResourceMap[srcResource];

        if(!dx12DstResource || !dx12SrcResource)
        {
            WD_CORE_ERROR("Resource not found in ResourceMap");
        }
        
        WorldCommandList.first->CopyResource(dx12DstResource, dx12SrcResource);
    }

    void DX12Renderer::CopyBufferRegion(GraphicResource* dstResource, size_t dstOffset, GraphicResource* srcResource, size_t srcOffset, size_t size)
    {
        auto lastStateDst = ResourceBarrier(dstResource, COPY_DEST);
        auto lastStateSrc = ResourceBarrier(srcResource, COPY_SOURCE);
        
        ID3D12Resource* dx12DstResource = ResourceMap[dstResource];
        ID3D12Resource* dx12SrcResource = ResourceMap[srcResource];

        if(!dx12DstResource || !dx12SrcResource)
        {
            WD_CORE_ERROR("Resource not found in ResourceMap");
        }

        WorldCommandList.first->CopyBufferRegion(dx12DstResource, dstOffset, dx12SrcResource, srcOffset, size);
        
        ResourceBarrier(dstResource, lastStateDst);
        ResourceBarrier(srcResource, lastStateSrc);
    }

    void DX12Renderer::UploadBuffer(Buffer* buffer, void* data, uint32_t size, uint offset)
    {
        ID3D12Resource* resource = ResourceMap[buffer];
        ID3D12Resource* uploadResource = ResourceMap[buffer->GetUploadResource()];
        D3D12_RESOURCE_STATES beforeState = (D3D12_RESOURCE_STATES)buffer->GetCurrentState();
        WorldCommandList.first->UpdateRes(resource, uploadResource, data, size, beforeState, offset);
    }

    void DX12Renderer::DownloadBuffer(Buffer* buffer, void* data, size_t size)
    {
        if(data)
        {
            auto readbackBuffer = buffer->GetReadbackResource();
            
            if(readbackBuffer)
            {
                auto resource = ResourceMap[readbackBuffer];
                
                if(resource)
                {
                    auto beforeState = buffer->GetCurrentState();
                    //copy the GPU buffer to the readback buffer
                    ResourceBarrier(buffer, beforeState, COPY_SOURCE);
                    CopyResource(readbackBuffer, buffer);
                    ResourceBarrier(buffer, COPY_SOURCE, beforeState);
                    //map and copy data
                    UINT8* pMappedData;
                    HRESULT hr = resource->Map(0, nullptr, reinterpret_cast<void**>(&pMappedData));
                    if(FAILED(hr))
                    {
                        DX12Helper::PrintHResultError(hr);
                    }
                    memcpy(data, pMappedData, size);
                    resource->Unmap(0, nullptr);
                }
                else
                {
                    WD_CORE_ERROR("DX12Resource not found for the bound readback resource");
                }
            }
            else
            {
                WD_CORE_ERROR("Current graphic resource doesnt have readback resource");
            }
        }
        else
        {
            WD_CORE_ERROR("Wrong destination for the readback");
        }
    }

    void DX12Renderer::ClearRenderTarget(RenderTarget* rt)
    {
        uint index = rt->GetIndex(RTV_DSV);
        auto handle = RTVHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += index * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        WorldCommandList.first->ClearRenderTarget(handle);
    }

    void DX12Renderer::ClearDepthStencil(RenderTarget* ds)
    {
        uint index = ds->GetIndex(RTV_DSV);
        auto handle = DSVHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += index * Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        WorldCommandList.first->ClearDepthStencil(handle);
    }

    void DX12Renderer::ResourceBarrier(GraphicResource* resource, ResourceStates before, ResourceStates after)
    {
        ID3D12Resource* dx12Resource = ResourceMap[resource];

        if(dx12Resource == nullptr)
        {
            WD_CORE_ERROR("Resource not found in ResourceMap");
        }

        resource->SetCurrentState(after);
        
        WorldCommandList.first->ResourceBarrier(dx12Resource, (D3D12_RESOURCE_STATES)before, (D3D12_RESOURCE_STATES)after);
    }

    ResourceStates DX12Renderer::ResourceBarrier(GraphicResource* resource, ResourceStates after)
    {
        auto currentState = resource->GetCurrentState();

        ResourceBarrier(resource, currentState, after);

        return currentState;
    }
}
