#include "wdpch.h"
#include "DX12Renderer.h"

#include <d3d12shader.h>
#include <d3dcompiler.h>

#include "D3DX12.h"
#include "DX12AccelerationStructure.h"
#include "DX12Buffer.h"
#include "DX12CommandSignature.h"
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
#include "Waldem/Editor/Editor.h"
#include "Waldem/Editor/UIStyles.h"

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
    
    void DX12Renderer::Initialize(CWindow* window)
    {
        CurrentWindow = window;
        
        int width = (int)CurrentWindow->GetWidth();
        int height = (int)CurrentWindow->GetHeight();
        
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

        InitializeUI();
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

        WArray<RenderTarget*> editorRenderTargets = { new DX12RenderTarget("EditorViewRT", Device, WorldCommandList.first, size.x, size.y, TextureFormat::R8G8B8A8_UNORM, ImGuiHeap, 1) };
        RenderTarget* editorViewportDepth = new DX12RenderTarget("EditorViewDepth", Device, WorldCommandList.first, size.x, size.y, TextureFormat::D32_FLOAT);
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
        
        WArray<RenderTarget*> gameRenderTargets = { new DX12RenderTarget("GameViewRT", Device, WorldCommandList.first, size.x, size.y, TextureFormat::R8G8B8A8_UNORM, ImGuiHeap, 2) };
        RenderTarget* gameViewportDepth = new DX12RenderTarget("GameViewDepth", Device, WorldCommandList.first, size.x, size.y, TextureFormat::D32_FLOAT);
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

            auto renderTarget = new DX12RenderTarget("MainViewport_FrameBuffer_" + std::to_string(i), Device, size.x, size.y, TextureFormat::R8G8B8A8_UNORM, resource);
            ResourceBarrier(renderTarget, ALL_SHADER_RESOURCE, PRESENT);

            if(FAILED(h))
            {
                throw std::runtime_error("Failed to get D3D12 SwapChain buffer");
            }

            mainFrameBuffer->AddRenderTarget(renderTarget);
        }
        mainFrameBuffer->SetDepth(new DX12RenderTarget("MainViewport_Depth", Device, WorldCommandList.first, size.x, size.y, TextureFormat::D32_FLOAT));
        
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

                    auto renderTarget = new DX12RenderTarget("MainViewport_FrameBuffer_" + std::to_string(i), Device, size.x, size.y, TextureFormat::R8G8B8A8_UNORM, resource);
                    ResourceBarrier(renderTarget, ALL_SHADER_RESOURCE, PRESENT);

                    if(FAILED(h))
                    {
                        throw std::runtime_error("Failed to get D3D12 SwapChain buffer");
                    }

                    MainViewport.FrameBuffer->AddRenderTarget(renderTarget);
                }

                MainViewport.FrameBuffer->SetDepth(new DX12RenderTarget("MainViewport_Depth", Device, WorldCommandList.first, size.x, size.y, TextureFormat::D32_FLOAT));
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

        RenderTarget* editorRenderTarget = { new DX12RenderTarget("EditorViewRT", Device, WorldCommandList.first, size.x, size.y, TextureFormat::R8G8B8A8_UNORM, ImGuiHeap, 1) };
        EditorViewport.FrameBuffer->AddRenderTarget(editorRenderTarget);

        RenderTarget* editorViewportDepth = new DX12RenderTarget("EditorViewDepth", Device, WorldCommandList.first, size.x, size.y, TextureFormat::D32_FLOAT);
        EditorViewport.FrameBuffer->SetDepth(editorViewportDepth);
    }

    void DX12Renderer::Draw(CModel* model)
    {
        auto& cmd = WorldCommandList.first;

        cmd->Draw(model);
    }

    void DX12Renderer::Draw(CMesh* mesh)
    {
        auto& cmd = WorldCommandList.first;

        cmd->Draw(mesh);
    }

    void DX12Renderer::DrawIndirect(CommandSignature* commandSignature, uint numCommands, Buffer* indirectBuffer)
    {
        auto& cmd = WorldCommandList.first;
        auto nativeCommandSignature = (ID3D12CommandSignature*)commandSignature->GetNativeObject();

        cmd->DrawIndirect(nativeCommandSignature, numCommands, indirectBuffer);
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

        ResourceBarrier(EditorViewport.FrameBuffer->GetCurrentRenderTarget(), ALL_SHADER_RESOURCE, RENDER_TARGET);
        ResourceBarrier(EditorViewport.FrameBuffer->GetDepth(), ALL_SHADER_RESOURCE, DEPTH_WRITE);
        
        if(!WorldCommandList.second)
        {
            worldCmd->BeginInternal(EditorViewport);
            
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
        auto& worldCmd = WorldCommandList.first;

        ResourceBarrier(MainViewport.FrameBuffer->GetCurrentRenderTarget(), PRESENT, RENDER_TARGET);
        ResourceBarrier(MainViewport.FrameBuffer->GetDepth(), ALL_SHADER_RESOURCE, DEPTH_WRITE);
        
        if(!WorldCommandList.second)
        {
            worldCmd->BeginInternal(MainViewport);
            
            WorldCommandList.second = true;
        }
        
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
    }

    void DX12Renderer::SetRootSignature(RootSignature* rootSignature)
    {
        WorldCommandList.first->SetRootSignature(rootSignature);
    }

    void DX12Renderer::SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil)
    {
        WorldCommandList.first->SetRenderTargets(renderTargets, depthStencil);
    }

    Pipeline* DX12Renderer::CreateGraphicPipeline(const WString& name, RootSignature* rootSignature, PixelShader* shader, WArray<TextureFormat> RTFormats, RasterizerDesc rasterizerDesc, DepthStencilDesc depthStencilDesc, PrimitiveTopologyType primitiveTopologyType, const WArray<InputLayoutDesc>& inputLayout)
    {
        return new DX12GraphicPipeline(name, Device, rootSignature, shader, RTFormats, rasterizerDesc, depthStencilDesc, primitiveTopologyType, inputLayout);
    }

    Pipeline* DX12Renderer::CreateComputePipeline(const WString& name, RootSignature* rootSignature, ComputeShader* shader)
    {
        return new DX12ComputePipeline(name, Device, rootSignature, shader);
    }

    Pipeline* DX12Renderer::CreateRayTracingPipeline(const WString& name, RootSignature* rootSignature, RayTracingShader* shader)
    {
        return new DX12RayTracingPipeline(name, Device, rootSignature, shader);
    }

    RootSignature* DX12Renderer::CreateRootSignature(WArray<GraphicResource> resources)
    {
        return new DX12RootSignature(Device, WorldCommandList.first, resources);
    }

    CommandSignature* DX12Renderer::CreateCommandSignature(RootSignature* rootSignature)
    {
        CommandSignature* commandSignature = new DX12CommandSignature(Device, rootSignature);
        return commandSignature;
    }

    Texture2D* DX12Renderer::CreateTexture(WString name, int width, int height, TextureFormat format, size_t dataSize, uint8_t* data)
    {
        Texture2D* texture = new DX12Texture(name, Device, WorldCommandList.first, width, height, format, dataSize, data);
        return texture;
    }

    Texture2D* DX12Renderer::CreateTexture(TextureDesc desc)
    {
        Texture2D* texture = new DX12Texture(Device, WorldCommandList.first, desc);
        return texture;
    }

    RenderTarget* DX12Renderer::CreateRenderTarget(WString name, int width, int height, TextureFormat format)
    {
        DX12RenderTarget* renderTarget = new DX12RenderTarget(name, Device, WorldCommandList.first, width, height, format);
        return renderTarget;
    }

    AccelerationStructure* DX12Renderer::CreateBLAS(WString name, WArray<RayTracingGeometry>& geometries)
    {
        return new DX12AccelerationStructure(name, Device, WorldCommandList.first, AccelerationStructureType::BottomLevel, geometries);
    }

    AccelerationStructure* DX12Renderer::CreateTLAS(WString name, WArray<RayTracingInstance>& instances)
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

    Buffer* DX12Renderer::CreateBuffer(WString name, BufferType type, void* data, uint32_t size, uint32_t stride)
    {
        return new DX12Buffer(Device, WorldCommandList.first, name, type, data, size, stride);
    }

    void DX12Renderer::UpdateBuffer(Buffer* buffer, void* data, uint32_t size)
    {
        WorldCommandList.first->UpdateBuffer(buffer, data, size);
    }

    void DX12Renderer::ClearRenderTarget(RenderTarget* rt)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dx12RenderTarget = ((DX12RenderTarget*)rt)->GetRTVHandle();
        WorldCommandList.first->ClearRenderTarget(dx12RenderTarget);
    }

    void DX12Renderer::ClearDepthStencil(RenderTarget* ds)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dx12DepthStencil = ((DX12RenderTarget*)ds)->GetRTVHandle();
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
}