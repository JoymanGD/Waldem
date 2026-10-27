using System;
using System.Runtime.InteropServices;

namespace Waldem
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4
    {
        public float x;
        public float y;
        public float z;
        public float w;

        public double magnitude => Length();
        public float magnitudeF => (float)Length();

        public Vector4(float x, float y, float z, float w)
        {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public static Vector4 Zero => new Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        public static Vector4 Up => new Vector4(0.0f, 1.0f, 0.0f, 0.0f);
        public static Vector4 Forward => new Vector4(0.0f, 0.0f, 1.0f, 0.0f);
        public static Vector4 Right => new Vector4(1.0f, 0.0f, 0.0f, 0.0f);

        public static Vector4 operator +(Vector4 left, Vector4 right)
        {
            return new Vector4(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
        }

        public static Vector4 operator -(Vector4 left, Vector4 right)
        {
            return new Vector4(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
        }

        public static Vector4 operator *(Vector4 value, float scalar)
        {
            return new Vector4(value.x * scalar, value.y * scalar, value.z * scalar, value.w * scalar);
        }

        public void Normalize()
        {
            float magnitude = (float)Length();
            if (magnitude > 0.0001f)
            {
                x /= magnitude;
                y /= magnitude;
                z /= magnitude;
                w /= magnitude;
            }
        }

        public double Length()
        {
            return System.Math.Sqrt(x * x + y * y + z * z + w * w);
        }
    }
}
