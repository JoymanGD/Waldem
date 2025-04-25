#pragma once
#include "Waldem/Types/MathTypes.h"

namespace Waldem
{
    class Plane
    {
    public:
        Plane();
        Plane(const Vector3 &normal, float distance, bool normalise = false);

        ~Plane() {}

        //Sets the planes normal, which should be UNIT LENGTH!!!
        void SetNormal(const Vector3 &normal) { Normal = normal; }
        //Gets the planes normal.
        Vector3 GetNormal() const { return Normal; }
        //Sets the planes distance from the origin
        void SetDistance(float dist) { Distance = dist; }
        //Gets the planes distance from the origin
        float GetDistance() const { return Distance; }
        //Performs a simple sphere / plane test
        bool SphereInPlane(const Vector3 &position, float radius) const;
        //Performs a simple sphere / point test
        bool PointInPlane(const Vector3 &position) const;

        float DistanceFromPlane(const Vector3 &in) const;

        Vector3 GetPointOnPlane() const
        {
            return Normal * -Distance;
        }

        Vector3 ProjectPointOntoPlane(const Vector3 &point) const;

        static Plane PlaneFromTri(const Vector3 &v0, const Vector3 &v1, const Vector3 &v2);

    protected:
        //Unit-length plane normal
        Vector3 Normal;
        //Distance from origin
        float Distance;
    };
}
