#pragma once
#include "Model/Line.h"
#include "Waldem/World/Camera.h"

namespace Waldem
{
    struct BoundingBox
    {
        Vector3 Min;
        Vector3 Max;

        BoundingBox GetTransformed(const Matrix4& transform)
        {
            BoundingBox result;
            result.Min = Vector3(transform * Vector4(Min, 1.0f));
            result.Max = Vector3(transform * Vector4(Max, 1.0f));

            return result;
        }

        void Transform(const Matrix4& transform)
        {
            Min = Vector3(transform * Vector4(Min, 1.0f));
            Max = Vector3(transform * Vector4(Max, 1.0f));
        }

        bool Intersects(const BoundingBox& other)
        {
            return Max.x >= other.Min.x && Min.x <= other.Max.x &&
                   Max.y >= other.Min.y && Min.y <= other.Max.y &&
                   Max.z >= other.Min.z && Min.z <= other.Max.z;
        }

        WArray<Line> GetLines(Vector4 color)
        {
            WArray<Line> lines;
            
            lines.Add({Vector3(Min.x, Min.y, Min.z), Vector3(Max.x, Min.y, Min.z), color});
            lines.Add({Vector3(Min.x, Min.y, Min.z), Vector3(Min.x, Max.y, Min.z), color});
            lines.Add({Vector3(Min.x, Min.y, Min.z), Vector3(Min.x, Min.y, Max.z), color});
            lines.Add({Vector3(Max.x, Max.y, Max.z), Vector3(Min.x, Max.y, Max.z), color});
            lines.Add({Vector3(Max.x, Max.y, Max.z), Vector3(Max.x, Min.y, Max.z), color});
            lines.Add({Vector3(Max.x, Max.y, Max.z), Vector3(Max.x, Max.y, Min.z), color});
            lines.Add({Vector3(Min.x, Max.y, Min.z), Vector3(Max.x, Max.y, Min.z), color});
            lines.Add({Vector3(Min.x, Max.y, Min.z), Vector3(Min.x, Max.y, Max.z), color});
            lines.Add({Vector3(Max.x, Min.y, Min.z), Vector3(Max.x, Min.y, Max.z), color});
            lines.Add({Vector3(Max.x, Min.y, Min.z), Vector3(Max.x, Max.y, Min.z), color});
            lines.Add({Vector3(Min.x, Min.y, Max.z), Vector3(Max.x, Min.y, Max.z), color});
            lines.Add({Vector3(Min.x, Min.y, Max.z), Vector3(Min.x, Max.y, Max.z), color});
            
            return lines;
        }

        void Expand(const BoundingBox& other)
        {
            Min.x = glm::min(Min.x, other.Min.x);
            Min.y = glm::min(Min.y, other.Min.y);
            Min.z = glm::min(Min.z, other.Min.z);
            Max.x = glm::max(Max.x, other.Max.x);
            Max.y = glm::max(Max.y, other.Max.y);
            Max.z = glm::max(Max.z, other.Max.z);
        }
        
        bool IsInFrustum(WArray<FrustumPlane>& frustrumPlanes)
        {
            //convert BoundingBox to Vector3 for simplicity
            Vector3 min = Vector3(Min.x, Min.y, Min.z);
            Vector3 max = Vector3(Max.x, Max.y, Max.z);

            for (const auto& plane : frustrumPlanes)
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
