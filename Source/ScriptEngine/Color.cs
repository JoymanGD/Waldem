using System;
using System.Runtime.InteropServices;

namespace Waldem
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Color
    {
        public float r;
        public float g;
        public float b;
        public float a;

        public Color(float r, float g, float b, float a)
        {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }

        public static Color operator +(Color left, Color right)
        {
            return new Color(left.r + right.r, left.g + right.g, left.b + right.b, left.a + right.a);
        }

        public static Color operator -(Color left, Color right)
        {
            return new Color(left.r - right.r, left.g - right.g, left.b - right.b, left.a - right.a);
        }

        public static Color operator *(Color value, float scalar)
        {
            return new Color(value.r * scalar, value.g * scalar, value.b * scalar, value.a * scalar);
        }
    }
}
