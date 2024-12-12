#pragma once
#include "Waldem/World/Camera.h"

namespace Waldem
{
    struct BoundingBox
    {
        Vector3 Min;
        Vector3 Max;

        BoundingBox Transform(Matrix4 transform)
        {
            BoundingBox result;
            result.Min = Vector3(transform * Vector4(Min, 1.0f));
            result.Max = Vector3(transform * Vector4(Max, 1.0f));

            return result;
        }
        
        bool IsInFrustum(const WArray<FrustumPlane>& frustumPlanes)
        {
            //convert BoundingBox to Vector3 for simplicity
            Vector3 min = Vector3(Min.x, Min.y, Min.z);
            Vector3 max = Vector3(Max.x, Max.y, Max.z);

            for (const auto& plane : frustumPlanes)
            {
                //check if all 8 corners of the AABB are behind the plane
                if (!plane.IsPointInFront(Vector3(min.x, min.y, min.z)) &&
                    !plane.IsPointInFront(Vector3(max.x, min.y, min.z)) &&
                    !plane.IsPointInFront(Vector3(min.x, max.y, min.z)) &&
                    !plane.IsPointInFront(Vector3(max.x, max.y, min.z)) &&
                    !plane.IsPointInFront(Vector3(min.x, min.y, max.z)) &&
                    !plane.IsPointInFront(Vector3(max.x, min.y, max.z)) &&
                    !plane.IsPointInFront(Vector3(min.x, max.y, max.z)) &&
                    !plane.IsPointInFront(Vector3(max.x, max.y, max.z)))
                {
                    return false; //completely outside this plane
                }
            }

            return true; //inside or intersecting the frustum
        }
    };
}
