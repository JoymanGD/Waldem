#pragma once
#include "Waldem/ECS/Components/ColliderComponent.h"
#include "Waldem/Renderer/Model/Simplex.h"

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
        static Vector3 Support(const ColliderComponent* colliderA, const ColliderComponent* colliderB, Transform& worldTransformA, Transform& worldTransformB, const Vector3& dir)
        {
            return colliderA->FindFurthestPoint(dir, worldTransformA) - colliderB->FindFurthestPoint(-dir, worldTransformB);
        }

        static bool SameDirection(const Vector3& a, const Vector3& b)
        {
            return dot(a, b) > 0;
        }

        static bool Line(Simplex& points, Vector3& direction)
        {
            Vector3 a = points[0];
            Vector3 b = points[1];

            Vector3 ab = b - a;
            Vector3 ao = -a;

            if(SameDirection(ab, ao))
            {
                direction = cross(cross(ab, ao), ab);
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

        static bool NextSimplex(Simplex& points, Vector3 direction)
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
