#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/Renderer/Model/Simplex.h"
#include "Waldem/Types/WMap.h"
#include "Waldem/Types/WPair.h"

namespace Waldem
{
    struct BVHNode
    {
        AABB Box;
        BVHNode* Left;
        BVHNode* Right;
        int ObjectIndex;
        String DebugName;

        bool IsLeaf() const { return Left == nullptr && Right == nullptr; }
    };
    
    struct Contact
    {
        Vector3 ContactPointA;
        Vector3 ContactPointB;
        Vector3 Normal;
        float PenetrationDepth;
        bool HasCollision;
    };

    struct CollisionPairHash
    {
        size_t operator()(const std::pair<int, int>& p) const
        {
            return std::hash<int>()(p.first) ^ std::hash<int>()(p.second);
        }
    };
    
    using CollisionPair = std::pair<int, int>;
    
    class WALDEM_API GeometryUtils
    {
    public:
        static Vector3 Support(const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform* worldTransformA, Transform* worldTransformB, const Vector3& dir)
        {
            return colliderA->FindFurthestPoint(dir, worldTransformA) - colliderB->FindFurthestPoint(-dir, worldTransformB);
        }

        static bool SameDirection(const Vector3& a, const Vector3& b)
        {
            return dot(a, b) > 0;
        }
        
        static std::pair<std::vector<Vector4>, size_t> GetFaceNormals(const std::vector<Vector3>& polytope, const std::vector<size_t>& faces)
        {
            std::vector<Vector4> normals;
            size_t minTriangle = 0;
            float  minDistance = FLT_MAX;

            for (size_t i = 0; i < faces.size(); i += 3)
            {
                Vector3 a = polytope[faces[i]];
                Vector3 b = polytope[faces[i + 1]];
                Vector3 c = polytope[faces[i + 2]];

                Vector3 normal = normalize(cross(b - a, c - a));
                float distance = dot(normal, a);

                if (distance < 0)
                {
                    normal   *= -1;
                    distance *= -1;
                }

                normals.emplace_back(normal, distance);

                if (distance < minDistance)
                {
                    minTriangle = i / 3;
                    minDistance = distance;
                }
            }

            return { normals, minTriangle };
        }

        static void AddIfUniqueEdge(std::vector<std::pair<size_t, size_t>>& edges, const std::vector<size_t>& faces, size_t a, size_t b)
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

        static bool Line(Simplex& points, Vector3& direction)
        {
            Vector3 a = points[0];
            Vector3 b = points[1];

            Vector3 ab = b - a;
            Vector3 ao = -a;

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
            Vector3 a = points[0];
            Vector3 b = points[1];
            Vector3 c = points[2];

            Vector3 ab = b - a;
            Vector3 ac = c - a;
            Vector3 ao = -a;

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
            Vector3 a = points[0];
            Vector3 b = points[1];
            Vector3 c = points[2];
            Vector3 d = points[3];

            Vector3 ab = b - a;
            Vector3 ac = c - a;
            Vector3 ad = d - a;
            Vector3 ao = -a;

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
