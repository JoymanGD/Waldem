#include "wdpch.h"
#include "Plane.h"

namespace Waldem
{
    Plane::Plane()
    {
        Normal = Vector3(0, 1, 0);
        Distance = 0.0f;
    };

    Plane::Plane(const Vector3 &normal, float distance, bool normalise)
    {
        if(normalise)
        {
            Normal = normalize(normal);

            Distance = distance	/ length(normal);
        }
        else
        {
            Normal = normal;
            Distance = distance;
        }
    }

    bool Plane::SphereInPlane(const Vector3 &position, float radius) const
    {
        if(dot(position, Normal)+ Distance <= -radius)
        {
            return false;
        }
        return true;	
    }

    bool Plane::PointInPlane(const Vector3 &position) const
    {
        if(dot(position, Normal)+ Distance < -0.001f)
        {
            return false;
        }

        return true;
    }

    Plane Plane::PlaneFromTri(const Vector3 &v0, const Vector3 &v1, const Vector3 &v2)
    {
        Vector3 v1v0 = v1-v0;
        Vector3 v2v0 = v2-v0;

        Vector3 normal = normalize(cross(v1v0,v2v0));
        
        float d = -dot(v0,normal);
        return Plane(normal,d,false);
    }

    float Plane::DistanceFromPlane(const Vector3 &in) const
    {
        return dot(in, Normal)+ Distance;
    }

    Vector3 Plane::ProjectPointOntoPlane(const Vector3 &point) const
    {
        float distance = DistanceFromPlane(point);

        return point - (Normal * distance);
    }
}