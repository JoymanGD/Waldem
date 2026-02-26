#pragma once
#include "Model/Line.h"
#include "Waldem/ECS/Components/Camera.h"
#include "Waldem/Types/WArray.h"

namespace Waldem
{
    COMPONENT()
    struct AABB
    {
        FIELD()
        Vector3 Min = Vector3(-0.5f, -0.5f, -0.5f);
        FIELD()
        Vector3 Max = Vector3(0.5f, 0.5f, 0.5f);

        AABB(){}
        AABB(const Vector3& min, const Vector3& max) : Min(min), Max(max) {}

        AABB GetTransformed(const Matrix4& transform)
        {
            glm::vec3 corners[8] =
            {
                glm::vec3(Min.x, Min.y, Min.z),
                glm::vec3(Max.x, Min.y, Min.z),
                glm::vec3(Min.x, Max.y, Min.z),
                glm::vec3(Max.x, Max.y, Min.z),
                glm::vec3(Min.x, Min.y, Max.z),
                glm::vec3(Max.x, Min.y, Max.z),
                glm::vec3(Min.x, Max.y, Max.z),
                glm::vec3(Max.x, Max.y, Max.z),
            };

            glm::vec3 transformedCorners[8];
            for (int i = 0; i < 8; ++i)
            {
                transformedCorners[i] = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
            }

            glm::vec3 newMin = transformedCorners[0];
            glm::vec3 newMax = transformedCorners[0];

            for (int i = 1; i < 8; ++i)
            {
                newMin.x = std::min(newMin.x, transformedCorners[i].x);
                newMin.y = std::min(newMin.y, transformedCorners[i].y);
                newMin.z = std::min(newMin.z, transformedCorners[i].z);

                newMax.x = std::max(newMax.x, transformedCorners[i].x);
                newMax.y = std::max(newMax.y, transformedCorners[i].y);
                newMax.z = std::max(newMax.z, transformedCorners[i].z);
            }

            AABB result;
            result.Min = newMin;
            result.Max = newMax;

            return result;
        }

        void Transform(const Matrix4& transform)
        {
            Min = Vector3(transform * Vector4(Min, 1.0f));
            Max = Vector3(transform * Vector4(Max, 1.0f));
        }

        bool Intersects(const AABB& other)
        {
            const float epsilon = 0.001f;
            return !(Max.x + epsilon < other.Min.x || Max.y + epsilon < other.Min.y || Max.z + epsilon < other.Min.z ||
                     Min.x - epsilon > other.Max.x || Min.y - epsilon > other.Max.y || Min.z - epsilon > other.Max.z);
        }

        float SurfaceArea() const
        {
            Vector3 size = Max - Min;
            return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
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

        WArray<Vector3> GetCorners()
        {
            WArray<Vector3> corners;
            
            corners.Add(Vector3(Min.x, Min.y, Min.z));
            corners.Add(Vector3(Max.x, Min.y, Min.z));
            corners.Add(Vector3(Min.x, Max.y, Min.z));
            corners.Add(Vector3(Max.x, Max.y, Min.z));
            corners.Add(Vector3(Min.x, Min.y, Max.z));
            corners.Add(Vector3(Max.x, Min.y, Max.z));
            corners.Add(Vector3(Min.x, Max.y, Max.z));
            corners.Add(Vector3(Max.x, Max.y, Max.z));
            
            return corners;
        }

        void Expand(const AABB& other)
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
