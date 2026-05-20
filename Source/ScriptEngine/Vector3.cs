using System;
using System.Runtime.InteropServices;

namespace Waldem
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3
    {
        public float x;
        public float y;
        public float z;

        public double magnitude => Length();

        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static Vector3 Zero => new Vector3(0.0f, 0.0f, 0.0f);

        public static Vector3 operator +(Vector3 left, Vector3 right)
        {
            return new Vector3(left.x + right.x, left.y + right.y, left.z + right.z);
        }

        public static Vector3 operator -(Vector3 left, Vector3 right)
        {
            return new Vector3(left.x - right.x, left.y - right.y, left.z - right.z);
        }

        public static Vector3 operator *(Vector3 value, float scalar)
        {
            return new Vector3(value.x * scalar, value.y * scalar, value.z * scalar);
        }

        public void Normalize()
        {
            float magnitude = (float)Math.Sqrt(x * x + y * y + z * z);
            if (magnitude > 0.0001f)
            {
                x /= magnitude;
                y /= magnitude;
                z /= magnitude;
            }
        }

        public double Length()
        {
            return Math.Sqrt(x * x + y * y + z * z);
        }
    }
}
