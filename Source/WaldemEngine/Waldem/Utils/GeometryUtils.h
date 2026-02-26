#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/Renderer/Model/Simplex.h"
#include "Waldem/Types/WMap.h"
#include "Waldem/Types/WPair.h"

namespace Waldem
{    
    class WALDEM_API GeometryUtils
    {
    public:
        static Vector3 Support(const ColliderComponent& colliderA, const ColliderComponent& colliderB, Transform& worldTransformA, Transform& worldTransformB, const Vector3& dir)
        {
            return colliderA.FindFurthestPoint(dir, worldTransformA) - colliderB.FindFurthestPoint(-dir, worldTransformB);
        }

        static bool SameDirection(const Vector3& a, const Vector3& b)
        {
            return dot(a, b) > 0;
        }
        
        static WPair<WArray<Vector4>, size_t> GetFaceNormals(const WArray<Vector3>& polytope, const WArray<uint>& faces)
        {
            WArray<Vector4> normals;
            size_t minFace = 0;
            float minDistance = FLT_MAX;

            for (size_t i = 0; i < faces.Num(); i += 3)
            {
                Vector3 a = polytope[faces[i]];
                Vector3 b = polytope[faces[i + 1]];
                Vector3 c = polytope[faces[i + 2]];

                Vector3 ab = b - a;
                Vector3 ac = c - a;
                Vector3 normal = normalize(cross(ab, ac));

                // Compute signed distance: n·x = d
                float distance = dot(normal, a);

                // Ensure normal points TOWARD origin (since origin is inside)
                // If distance < 0, origin is on the side the normal points to → good.
                // But we want d > 0 and n pointing from face to origin.
                // So if distance < 0, flip to make d positive and n → origin.
                if (distance < 0) {
                    normal = -normal;
                    distance = -distance;
                }

                // Guard against degenerate faces
                if (length(ab) < 1e-6f || length(ac) < 1e-6f || distance < 1e-6f)
                {
                    continue; // skip
                }

                normals.EmplaceBack(Vector4(normal, distance));

                if (distance > 0 && distance < minDistance)
                {
                    minDistance = distance;
                    minFace = normals.Num() - 1;
                }
            }

            if (normals.IsEmpty())
            {
                return { WArray<Vector4>(), 0 };
            }

            return { normals, minFace };
        }

        static WArray<Vector3> ClipPolygonToPlane(const WArray<Vector3>& inputVerts, const Plane& plane)
        {
            WArray<Vector3> outputVerts;

            if (inputVerts.Num() < 2)
                return outputVerts;

            const Vector3 planeNormal = plane.GetNormal();
            const float planeDist = plane.GetDistance();

            for (size_t i = 0; i < inputVerts.Num(); ++i)
            {
                const Vector3& current = inputVerts[i];
                const Vector3& next = inputVerts[(i + 1) % inputVerts.Num()];

                float distCurrent = dot(planeNormal, current) + planeDist;
                float distNext    = dot(planeNormal, next) + planeDist;

                bool currentInside = distCurrent <= 0.0f;
                bool nextInside    = distNext <= 0.0f;

                if (currentInside)
                    outputVerts.Add(current);

                if (currentInside != nextInside)
                {
                    float t = distCurrent / (distCurrent - distNext);
                    Vector3 intersection = current + t * (next - current);
                    outputVerts.Add(intersection);
                }
            }

            return outputVerts;
        }

        static void AddIfUniqueEdge(std::vector<std::pair<size_t, size_t>>& edges, const WArray<uint>& faces, size_t a, size_t b)
        {
            auto reverse = std::find(edges.begin(), edges.end(), std::make_pair(faces[b], faces[a]));
 
            if (reverse != edges.end())
            {
                edges.erase(reverse);
            }
            else
            {
                edges.emplace_back(faces[a], faces[b]);
            }
        }

        struct BarycentricResult
        {
            float u, v, w;
        };

        static constexpr float Epsilon = 1e-8f;

        static BarycentricResult GetBarycentricCoordinates(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
        {
            Vector3 v0 = b - a;
            Vector3 v1 = c - a;
            Vector3 v2 = p - a;

            float d00 = dot(v0, v0);
            float d01 = dot(v0, v1);
            float d11 = dot(v1, v1);
            float d20 = dot(v2, v0);
            float d21 = dot(v2, v1);
            float denom = d00 * d11 - d01 * d01;

            float u = 0.0f, v = 0.0f, w = 0.0f;

            if (fabs(denom) <= Epsilon)
            {
                if (d00 <= Epsilon && d11 <= Epsilon)
                {
                    u = 1.0f; v = 0.0f; w = 0.0f;
                }
                else
                {

                    if (d00 > Epsilon)
                    {
                        float t = d20 / d00;
                        u = 1.0f - t;
                        v = t;
                        w = 0.0f;
                    }
                    else if (d11 > Epsilon)
                    {
                        float t = d21 / d11;
                        u = 1.0f - t;
                        v = 0.0f;
                        w = t;
                    }
                    else
                    {
                        u = 1.0f; v = 0.0f; w = 0.0f;
                    }
                }
            }
            else
            {
                v = (d11 * d20 - d01 * d21) / denom;
                w = (d00 * d21 - d01 * d20) / denom;
                u = 1.0f - v - w;
            }

            return {u, v, w};
        }

        static bool Line(Simplex& points, Vector3& direction)
        {
            SimplexVertex a = points[0];
            SimplexVertex b = points[1];

            Vector3 ab = b.Point - a.Point;
            Vector3 ao = -a.Point;

            if(SameDirection(ab, ao))
            {
                Vector3 ABCrossAO = cross(ab, ao);
                direction = cross(ABCrossAO, ab);
            }
            else
            {
                points = { a };
                direction = ao;
            }

            return false;
        }

        static bool Triangle(Simplex& points, Vector3& direction)
        {
            SimplexVertex a = points[0];
            SimplexVertex b = points[1];
            SimplexVertex c = points[2];

            Vector3 ab = b.Point - a.Point;
            Vector3 ac = c.Point - a.Point;
            Vector3 ao = -a.Point;

            Vector3 abc = cross(ab, ac);

            if(SameDirection(cross(abc, ac), ao))
            {
                if(SameDirection(ac, ao))
                {
                    points = { a, c };
                    direction = cross(cross(ac, ao), ac);
                }
                else
                {
                    return Line(points = {a,b}, direction);
                }
            }
            else
            {
                if(SameDirection(cross(ab, abc), ao))
                {
                    return Line(points = { a, b }, direction);
                }
                else
                {
                    if(SameDirection(abc, ao))
                    {
                        direction = abc;
                    }
                    else
                    {
                        points = { a, c, b };
                        direction = -abc;
                    }
                }
            }

            return false;
        }

        static bool Tetrahedron(Simplex& points, Vector3& direction)
        {
            SimplexVertex a = points[0];
            SimplexVertex b = points[1];
            SimplexVertex c = points[2];
            SimplexVertex d = points[3];

            Vector3 ab = b.Point - a.Point;
            Vector3 ac = c.Point - a.Point;
            Vector3 ad = d.Point - a.Point;
            Vector3 ao = -a.Point;

            Vector3 abc = cross(ab, ac);
            Vector3 acd = cross(ac, ad);
            Vector3 adb = cross(ad, ab);

            if(SameDirection(abc, ao))
            {
                return Triangle(points = { a, b, c }, direction);
            }

            if(SameDirection(acd, ao))
            {
                return Triangle(points = { a, c, d }, direction);
            }

            if(SameDirection(adb, ao))
            {
                return Triangle(points = { a, d, b }, direction);
            }

            return true;
        }

        static bool NextSimplex(Simplex& points, Vector3& direction)
        {
            switch(points.Num())
            {
                case 2: return Line(points, direction);
                case 3: return Triangle(points, direction);
                case 4: return Tetrahedron(points, direction);
            }

            return false;
        }
    };
}
