#pragma once

namespace Waldem
{
    class WALDEM_API PhysicsUtils
    {
    public:
        // Utility function to compute volume of a tetrahedron from origin
        static float TetrahedronVolume(const Vector3& v0, const Vector3& v1, const Vector3& v2)
        {
            return dot(v0, cross(v1, v2)) / 6.0f;
        }

    	float static Determinant(const Matrix3 &m)
		{
		    return m[0][0] * m[1][1] * m[2][2] + m[1][0] * m[2][1] * m[0][2] + m[2][0] * m[0][1] * m[1][2] - m[0][0] * m[2][1] * m[1][2] - m[1][0] * m[0][1] * m[2][2] - m[2][0] * m[1][1] * m[0][2];
		}
        
        static Matrix3 ComputeMeshInertiaTensor(const WArray<Vector3>& vertices, const WArray<uint>& indices, float mass)
        {
			// count is the number of triangles (tris)
			// The moments are calculated based on the center of rotation (com) which defaults to [0,0,0] if unsupplied
			// assume mass==1.0  you can multiply by mass later.
			// for improved accuracy the next 3 variables, the determinant d, and its calculation should be changed to double
			float volume = 0; // technically this variable accumulates the volume times 6
			Vector3 diag(0, 0, 0); // accumulate matrix main diagonal integrals [x*x, y*y, z*z]
			Vector3 offd(0, 0, 0); // accumulate matrix off-diagonal  integrals [y*z, x*z, x*y]
			for (size_t i = 0; i < indices.Num(); i+= 3) // for each triangle
			{
				Matrix3 A(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]); // matrix trick for volume calc by taking determinant
				float d = Determinant(A);                                                                        // vol of tiny parallelapiped= d * dr * ds * dt (the 3 partials of my tetral triple integral equasion)
				volume += d;                                                                                     // add vol of current tetra (note it could be negative - that's ok we need that sometimes)
				for (int j = 0; j < 3; j++)
				{
					int j1 = (j + 1) % 3;
					int j2 = (j + 2) % 3;
					diag[j] += (A[0][j] * A[1][j] + A[1][j] * A[2][j] + A[2][j] * A[0][j] +
								A[0][j] * A[0][j] + A[1][j] * A[1][j] + A[2][j] * A[2][j]) *
							   d;  // divide by 60.0f later;
					offd[j] += (A[0][j1] * A[1][j2] + A[1][j1] * A[2][j2] + A[2][j1] * A[0][j2] +
								A[0][j1] * A[2][j2] + A[1][j1] * A[0][j2] + A[2][j1] * A[1][j2] +
								A[0][j1] * A[0][j2] * 2 + A[1][j1] * A[1][j2] * 2 + A[2][j1] * A[2][j2] * 2) *
							   d;  // divide by 120.0f later
				}
			}
        	
			diag /= volume * (60.0f / 6.0f);  // divide by total volume (vol/6) since density=1/volume
			offd /= volume * (120.0f / 6.0f);
   
        	Matrix3 inertia = Matrix3(diag.y + diag.z, -offd.z, -offd.y,
							-offd.z, diag.x + diag.z, -offd.x,
							-offd.y, -offd.x, diag.x + diag.y);
        	
			return inertia * mass;
        }
    };
}