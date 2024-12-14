#include "wdpch.h"
#include "DX12GraphicCommandList.h"

#include <d3dcompiler.h>

#include "D3DX12.h"
#include "DX12Buffer.h"
#include "DX12Helper.h"
#include "DX12Pipeline.h"
#include "DX12PixelShader.h"
#include "DX12RenderTarget.h"
#include "DX12RootSignature.h"
#include "Waldem/Renderer/Line.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    DX12GraphicCommandList::DX12GraphicCommandList(ID3D12Device* device)
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

        InitializeLineRendering();
    }

    DX12GraphicCommandList::~DX12GraphicCommandList()
    {
        CloseHandle(FenceEvent);
    }

    bool DX12GraphicCommandList::CompileFromFile(const String& shaderName)
    {
        String entryPoint = "main"; //TODO: make this configurable?
        
        auto currentPath = GetCurrentFolder();
        
        std::wstring wCurrentPath = std::wstring(currentPath.begin(), currentPath.end());
        std::wstring wShaderName = std::wstring(shaderName.begin(), shaderName.end());
        
        size_t lastSlash = wShaderName.find_last_of(L"/\\");

        // Extract the directory part
        std::wstring pathToShaders = wCurrentPath + L"/Shaders/";

        // Extract the base name
        std::wstring baseName = wShaderName.substr(lastSlash + 1);
        
        std::wstring shaderPath = pathToShaders + baseName + L".vs.hlsl";
        String target = "vs_5_1";

        //vertex shader
        HRESULT hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &LineVertexShaderBlob, // Output shader bytecode
            &LineShaderErrorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (LineShaderErrorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)LineShaderErrorBlob->GetBufferPointer());
            }
            
            return false;
        }

        //pixel shader
        shaderPath = pathToShaders + baseName + L".ps.hlsl";
        target = "ps_5_1";
        
        hr = D3DCompileFromFile(
            shaderPath.c_str(), // Filename
            nullptr, // Macros
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), // Entry point function (e.g., "main")
            target.c_str(), // Target profile (e.g., "vs_5_0" for vertex shader, "ps_5_0" for pixel shader)
            D3DCOMPILE_DEBUG, // Compile flags
            0,
            &LinePixelShaderBlob, // Output shader bytecode
            &LineShaderErrorBlob); // Output error messages

        if(FAILED(hr))
        {
            if (LineShaderErrorBlob)
            {
                WD_CORE_ERROR("Shader compilation error: {0}", (char*)LineShaderErrorBlob->GetBufferPointer());
            }
            
            return false;
        }

        return true;
    }

    void DX12GraphicCommandList::InitializeLineRendering()
    {
        if(CompileFromFile("Line"))
        {
            D3D12_ROOT_PARAMETER rootParameters[1] = {};

            rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParameters[0].Descriptor.ShaderRegister = 0;
            rootParameters[0].Descriptor.RegisterSpace = 0;
            rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
            rootSignatureDesc.NumParameters = 1;
            rootSignatureDesc.pParameters = rootParameters;
            rootSignatureDesc.NumStaticSamplers = 0;
            rootSignatureDesc.pStaticSamplers = nullptr;
            rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ID3DBlob* signatureBlob;
            ID3DBlob* errorBlob;
            HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
            if (FAILED(hr))
            {
            }

            hr = Device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&LineRootSignature));
            if (FAILED(hr))
            {
            }
            
            D3D12_HEAP_PROPERTIES uploadHeapProps = {};
            uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            uploadHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            uploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            uploadHeapProps.CreationNodeMask = 1;
            uploadHeapProps.VisibleNodeMask = 1;
        
            D3D12_HEAP_PROPERTIES defaultHeapProps = {};
            defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
            defaultHeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            defaultHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            defaultHeapProps.CreationNodeMask = 1;
            defaultHeapProps.VisibleNodeMask = 1;
            
            D3D12_RESOURCE_DESC vertexBufferDesc = {};
            vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            vertexBufferDesc.Width = sizeof(Line) * 100;
            vertexBufferDesc.Height = 1;
            vertexBufferDesc.DepthOrArraySize = 1;
            vertexBufferDesc.MipLevels = 1;
            vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
            vertexBufferDesc.SampleDesc.Count = 1;
            vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            Device->CreateCommittedResource(
                &defaultHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &vertexBufferDesc,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                nullptr,
                IID_PPV_ARGS(&LineVertexBuffer)
            );

            Device->CreateCommittedResource(
                &uploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &vertexBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&LineVertexBufferUpload)
            );

            D3D12_INPUT_ELEMENT_DESC inputLayout[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = LineRootSignature;
            psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
            psoDesc.VS = { LineVertexShaderBlob->GetBufferPointer(), LineVertexShaderBlob->GetBufferSize() };
            psoDesc.PS = { LinePixelShaderBlob->GetBufferPointer(), LinePixelShaderBlob->GetBufferSize() };
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
            psoDesc.RasterizerState.DepthBias = 0;
            psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
            psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
            psoDesc.RasterizerState.MultisampleEnable = FALSE;
            psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
            psoDesc.RasterizerState.DepthClipEnable = TRUE;
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = TRUE;
            psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            psoDesc.SampleDesc.Quality = 0;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&LinePipeline));
        }
    }

    void DX12GraphicCommandList::BeginInternal(D3D12_VIEWPORT* viewport, D3D12_RECT* scissor, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle)
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

    void DX12GraphicCommandList::EndInternal()
    {
        if(!Lines.IsEmpty())
        {
            DrawLines(Lines);
            Lines.Clear();
        }
        
        CurrentExecutableShader = nullptr;
    }

    void DX12GraphicCommandList::Draw(Model* model)
    {
        //Draw meshes
        auto meshes = model->GetMeshes();

        for (auto mesh : meshes)
        {
            Draw(mesh);
        }
    }

    void DX12GraphicCommandList::Draw(Mesh* mesh)
    {
        //Draw mesh
        auto& indexBufferView = ((DX12IndexBuffer*)mesh->IB)->GetBufferView();
        auto& vertexBufferView = ((DX12VertexBuffer*)mesh->VB)->GetBufferView();
        CommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        CommandList->IASetIndexBuffer(&indexBufferView);
        CommandList->DrawIndexedInstanced(mesh->IB->GetCount(), 1, 0, 0, 0);
    }

    void DX12GraphicCommandList::AddLine(Line line)
    {
        Lines.Add(line);
    }

    void DX12GraphicCommandList::DrawLines(WArray<Line> lines)
    {
        auto vertexBufferSize = lines.GetSize();
        void* mappedData = nullptr;
        LineVertexBufferUpload->Map(0, nullptr, &mappedData);
        memcpy(mappedData, lines.GetData(), vertexBufferSize);
        LineVertexBufferUpload->Unmap(0, nullptr);

        ResourceBarrier(LineVertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

        CommandList->CopyBufferRegion(LineVertexBuffer, 0, LineVertexBufferUpload, 0, vertexBufferSize);

        ResourceBarrier(LineVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        
        D3D12_VERTEX_BUFFER_VIEW vbView;
        vbView.BufferLocation = LineVertexBuffer->GetGPUVirtualAddress(); 
        vbView.SizeInBytes = vertexBufferSize;
        vbView.StrideInBytes = sizeof(LineVertex);
        
        CommandList->SetPipelineState(LinePipeline);
        CommandList->SetGraphicsRootSignature(LineRootSignature);
        CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
        CommandList->IASetVertexBuffers(0, 1, &vbView);
        CommandList->DrawInstanced(lines.Num() * 2, 1, 0, 0);
    }

    void DX12GraphicCommandList::Clear(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget, D3D12_CPU_DESCRIPTOR_HANDLE depthStencil, Vector3 clearColor)
    {
        //clear the render target
        const float clearColorFloat[] = { clearColor.x, clearColor.y, clearColor.z, 1.0f };
        CommandList->ClearRenderTargetView(renderTarget, clearColorFloat, 0, nullptr);
        CommandList->ClearDepthStencilView(depthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    void DX12GraphicCommandList::SetPipeline(Pipeline* pipeline)
    {
        DX12Pipeline* dx12Pipeline = (DX12Pipeline*)pipeline;
        D3D12_GRAPHICS_PIPELINE_STATE_DESC* psoDesc = dx12Pipeline->GetDesc();
        ID3D12PipelineState* pso = (ID3D12PipelineState*)dx12Pipeline->GetNativeObject();
        CommandList->SetPipelineState(pso);

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
    }

    void DX12GraphicCommandList::SetRootSignature(RootSignature* rootSignature)
    {
        DX12RootSignature* dx12RootSignature = (DX12RootSignature*)rootSignature;
        ID3D12RootSignature* rootSignatureObject = (ID3D12RootSignature*)dx12RootSignature->GetNativeObject();
        CommandList->SetGraphicsRootSignature(rootSignatureObject);

        auto resourcesHeap = dx12RootSignature->GetResourcesHeap();
        auto samplersHeap = dx12RootSignature->GetSamplersHeap();
        auto rootParamTypes = dx12RootSignature->GetRootParamTypes();
        
        ID3D12DescriptorHeap* heaps[] = { resourcesHeap, samplersHeap };
        CommandList->SetDescriptorHeaps(2, heaps);
        
        UINT descriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = resourcesHeap->GetGPUDescriptorHandleForHeapStart();
        UINT samplerDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        D3D12_GPU_DESCRIPTOR_HANDLE samplersHandle = samplersHeap->GetGPUDescriptorHandleForHeapStart();

        for (uint32_t i = 0; i < rootParamTypes.Num(); ++i)
        {
            auto& rootParamType = rootParamTypes[i];

            if(rootParamType == RTYPE_Sampler)
            {
                CommandList->SetGraphicsRootDescriptorTable(i, samplersHandle);
                samplersHandle.ptr += samplerDescriptorSize;
            }
            else if(rootParamType == RTYPE_Constant)
            {
                continue;
            }
            else
            {
                CommandList->SetGraphicsRootDescriptorTable(i, handle);
                handle.ptr += descriptorSize;
            }
        }
    }

    void DX12GraphicCommandList::SetRenderTargets(WArray<RenderTarget*> renderTargets, RenderTarget* depthStencil)
    {
        RenderTarget* currentRT = renderTargets.IsEmpty() ? depthStencil : renderTargets[0];
        
        if(!renderTargets.IsEmpty() || depthStencil)
        {
            D3D12_VIEWPORT Viewport = {};
            D3D12_RECT ScissorRect = {};
                
            Viewport.Width = currentRT->GetWidth();
            Viewport.Height = currentRT->GetHeight();
            Viewport.MinDepth = 0;
            Viewport.MaxDepth = 1;
                
            ScissorRect.right = currentRT->GetWidth();
            ScissorRect.bottom = currentRT->GetHeight();
                
            CommandList->RSSetViewports(1, &Viewport);
            CommandList->RSSetScissorRects(1, &ScissorRect);
            
            WArray<D3D12_CPU_DESCRIPTOR_HANDLE> rtHandles;
            for(auto renderTarget : renderTargets)
            {
                auto handle = ((DX12RenderTarget*)renderTarget)->GetRenderTargetHandle();
                rtHandles.Add(handle);
            }

            D3D12_CPU_DESCRIPTOR_HANDLE dsHandle = {};

            if(depthStencil)
                dsHandle = ((DX12RenderTarget*)depthStencil)->GetRenderTargetHandle();
                
            CommandList->OMSetRenderTargets(rtHandles.Num(), rtHandles.GetData(), FALSE, depthStencil ? &dsHandle : nullptr);
        }
        else
        {
            CommandList->RSSetViewports(1, &CurrentViewport);
            CommandList->RSSetScissorRects(1, &CurrentScissorRect);
            CommandList->OMSetRenderTargets(1, &CurrentRenderTargetHandle, FALSE, &CurrentDepthStencilHandle);
        }
    }

    void DX12GraphicCommandList::Reset()
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

    void DX12GraphicCommandList::SetConstants(uint32_t slot, uint32_t numConstants, void* data)
    {
        CommandList->SetGraphicsRoot32BitConstants(slot, numConstants, data, 0);
    }

    void DX12GraphicCommandList::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const D3D12_TEXTURE_COPY_LOCATION* src, const D3D12_BOX* srcBox)
    {
        CommandList->CopyTextureRegion(dst, dstX, dstY, dstZ, src, srcBox);
    }

    void DX12GraphicCommandList::CopyResource(ID3D12Resource* dst, ID3D12Resource* src)
    {
        CommandList->CopyResource(dst, src);
    }

    void DX12GraphicCommandList::UpdateSubresoures(ID3D12Resource* destResource, ID3D12Resource* srcResource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
    {
        UpdateSubresources(CommandList, destResource, srcResource, 0, 0, numSubresources, subresourceData);
    }

    void DX12GraphicCommandList::Execute(ID3D12CommandQueue* commandQueue)
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

    void DX12GraphicCommandList::WaitForCompletion()
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

    void DX12GraphicCommandList::ResourceBarrier(uint32_t count, D3D12_RESOURCE_BARRIER* barrier)
    {
        CommandList->ResourceBarrier(count, barrier);
    }

    void DX12GraphicCommandList::ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after; // Or your desired state
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        CommandList->ResourceBarrier(1, &barrier);
    }

    void DX12GraphicCommandList::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle)
    {
        const float clearColorFloat[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        CommandList->ClearRenderTargetView(renderTargetHandle, clearColorFloat, 0, 0);
    }

    void DX12GraphicCommandList::ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle)
    {
        CommandList->ClearDepthStencilView(depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    }

    void DX12GraphicCommandList::Close()
    {
        //close the command list to indicate we're done recording commands.
        HRESULT hr = CommandList->Close();
        
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to close command list!");
        }
    }
}
