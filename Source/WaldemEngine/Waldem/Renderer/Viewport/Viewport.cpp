#include <wdpch.h>
#include "Viewport.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/Renderer/FrameBuffer.h"
#include "Waldem/Renderer/GBuffer.h"

namespace Waldem
{
    SViewport::SViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, int frameBufferSize, bool gbuffer) : Type(type), Name(name), Position(position), Size(size), DepthRange(depthRange)
    {
        FrameBuffer = new SFrameBuffer(name, frameBufferSize, size);

        if(gbuffer)
        {
            GBuffer = new SGBuffer(size);
        }
    }
    
    SViewport::SViewport(ViewportType type, WString name, Point2 position, Point2 size, Point2 depthRange, SFrameBuffer* frameBuffer, bool gbuffer) : Type(type), Name(name), Position(position), Size(size), DepthRange(depthRange)
    {
        FrameBuffer = frameBuffer;

        if(gbuffer)
        {
            GBuffer = new SGBuffer(size);
        }
    }

    void SViewport::SetViewport(Point2 position, Point2 size, Point2 depthRange)
    {
        Position = position;
        Size = size;
        DepthRange = depthRange;
    }

    ECS::Entity SViewport::GetLinkedCamera() const
    {
        return LinkedCamera;
    }

    bool SViewport::TryGetLinkedCamera(ECS::Entity& outCamera)
    {
        if (LinkedCamera.is_valid())
        {
            outCamera = LinkedCamera;
            return true;
        }
        return false;
    }

    void SViewport::LinkCamera(ECS::Entity camera)
    {
        LinkedCamera = camera;
    }

    void SViewport::UnlinkCamera()
    {
        LinkedCamera = {};
    }

    void SViewport::RequestResize(Point2 size)
    {
        PendingResize = true;
        PendingResizeTarget = size;
    }

    void SViewport::ApplyPendingResize()
    {
        if (PendingResize)
        {
            Resize(PendingResizeTarget);
            PendingResize = false;
        }
    }
    
    void SViewport::Resize(Point2 size)
    {
        Size = size;

        if(ResizeFunction)
        {
            ResizeFunction(size);
        }
        else
        {
            FrameBuffer->Resize(size);
            
            if(GBuffer)
            {
                GBuffer->Resize(size);
            }
            
            for (auto& callback : ResizeCallbacks)
            {
                callback(size);
            }
        }

        if(LinkedCamera)
        {
            auto& camera = LinkedCamera.get_mut<Camera>();
            camera.UpdateProjectionMatrix(camera.FieldOfView, size.x/(float)size.y, camera.NearPlane, camera.FarPlane);
            LinkedCamera.modified<Camera>(); 
        }
    }

    void SViewport::Move(Point2 position)
    {
        Position = position;
    }

    void SViewport::SetResizeFunction(const ResizeCallback& callback)
    {
        ResizeFunction = callback;
    }

    void SViewport::SubscribeOnResize(const ResizeCallback& callback)
    {
        ResizeCallbacks.Add(callback);
    }

    Point2 SViewport::TransformMousePosition(Point2 mousePos)
    {
        Point2 transformedPos = mousePos - Position;

        return transformedPos;
    }

    SGBuffer* SViewport::GetGBuffer() { return GBuffer; }
    
    RenderTarget* SViewport::GetGBufferRenderTarget(GBufferRenderTarget rt)
    {
        return GBuffer ? GBuffer->GetRenderTarget(rt) : nullptr;
    }
}
