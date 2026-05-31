using System;
using System.Runtime.InteropServices;

namespace Waldem
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Quaternion
    {
        public float w;
        public float x;
        public float y;
        public float z;

        public Quaternion(float w, float x, float y, float z)
        {
            this.w = w;
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static Quaternion Identity => new Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

        public double magnitude => Length();
        public float magnitudeF => (float)Length();
        public Vector3 EulerAngles
        {
            get
            {
                InternalCalls.Quaternion_ToEuler(ref this, out Vector3 result);
                return result;
            }
        }

        public double Length()
        {
            return System.Math.Sqrt(w * w + x * x + y * y + z * z);
        }

        public void Normalize()
        {
            this = Normalized(this);
        }

        public static Quaternion Normalized(Quaternion value)
        {
            InternalCalls.Quaternion_Normalize(ref value, out Quaternion result);
            return result;
        }

        public static Quaternion Euler(float x, float y, float z)
        {
            return Euler(new Vector3(x, y, z));
        }

        public static Quaternion Euler(Vector3 eulerDegrees)
        {
            InternalCalls.Quaternion_Euler(ref eulerDegrees, out Quaternion result);
            return result;
        }

        public static Quaternion AngleAxis(float angleDegrees, Vector3 axis)
        {
            InternalCalls.Quaternion_AngleAxis(angleDegrees, ref axis, out Quaternion result);
            return result;
        }

        public static Quaternion LookRotation(Vector3 forward)
        {
            return LookRotation(forward, new Vector3(0.0f, 1.0f, 0.0f));
        }

        public static Quaternion LookRotation(Vector3 forward, Vector3 up)
        {
            InternalCalls.Quaternion_LookRotation(ref forward, ref up, out Quaternion result);
            return result;
        }

        public static Quaternion operator *(Quaternion left, Quaternion right)
        {
            InternalCalls.Quaternion_Multiply(ref left, ref right, out Quaternion result);
            return result;
        }

        public static Vector3 operator *(Quaternion rotation, Vector3 point)
        {
            InternalCalls.Quaternion_RotateVector(ref rotation, ref point, out Vector3 result);
            return result;
        }

        public override string ToString()
        {
            return $"({w}, {x}, {y}, {z})";
        }
    }
}
