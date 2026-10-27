using System;
using System.Runtime.InteropServices;

namespace Waldem
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float x;
        public float y;

        public double magnitude => Length();
        public float magnitudeF => (float)Length();

        public Vector2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public static Vector2 Zero => new Vector2(0.0f, 0.0f);
        public static Vector2 Up => new Vector2(0.0f, 1.0f);
        public static Vector2 Right => new Vector2(1.0f, 0.0f);

        public static Vector2 operator +(Vector2 left, Vector2 right)
        {
            return new Vector2(left.x + right.x, left.y + right.y);
        }

        public static Vector2 operator -(Vector2 left, Vector2 right)
        {
            return new Vector2(left.x - right.x, left.y - right.y);
        }

        public static Vector2 operator *(Vector2 value, float scalar)
        {
            return new Vector2(value.x * scalar, value.y * scalar);
        }

        public void Normalize()
        {
            float magnitude = (float)Length();
            if (magnitude > 0.0001f)
            {
                x /= magnitude;
                y /= magnitude;
            }
        }

        public double Length()
        {
            return System.Math.Sqrt(x * x + y * y);
        }
    }
}
