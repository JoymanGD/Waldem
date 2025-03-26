#pragma once

namespace Waldem
{
    class WALDEM_API PhysicsUtils
    {
    public:
        static Matrix3 ComputeMeshInertiaTensor(const WArray<Vector3>& vertices, const WArray<uint>& indices, float mass)
        {
            Matrix3 inertia(0.0f);
            float volume = 0.0f;
            Vector3 centroid(0.0f);

            for (size_t i = 0; i < indices.Num(); i += 3)
            {
                Vector3 p0 = vertices[indices[i]];
                Vector3 p1 = vertices[indices[i + 1]];
                Vector3 p2 = vertices[indices[i + 2]];

                float tetraVolume = dot(p0, cross(p1, p2)) / 6.0f;
                volume += tetraVolume;
                centroid += tetraVolume * (p0 + p1 + p2) / 4.0f;

                Matrix3 temp = tetraVolume / 10.0f * (
                    Matrix3(
                        p0.x*p0.x + p1.x*p1.x + p2.x*p2.x, p0.x*p0.y + p1.x*p1.y + p2.x*p2.y, p0.x*p0.z + p1.x*p1.z + p2.x*p2.z,
                        p0.y*p0.x + p1.y*p1.x + p2.y*p2.x, p0.y*p0.y + p1.y*p1.y + p2.y*p2.y, p0.y*p0.z + p1.y*p1.z + p2.y*p2.z,
                        p0.z*p0.x + p1.z*p1.x + p2.z*p2.x, p0.z*p0.y + p1.z*p1.y + p2.z*p2.y, p0.z*p0.z + p1.z*p1.z + p2.z*p2.z
                    )
                );

                inertia += temp;
            }

            centroid /= volume;

            // Shift inertia tensor to centroid
            inertia = inertia - volume * (dot(centroid, centroid) * Matrix3(1.0f) - outerProduct(centroid, centroid));

            inertia *= mass / volume;

            return inertia;
        }
    };
}
