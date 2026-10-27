using System.Runtime.InteropServices;

namespace Waldem
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Curve
    {
        const uint MaxPoints = 5;

        private Vector2 Point0;
        private Vector2 Point1;
        private Vector2 Point2;
        private Vector2 Point3;
        private Vector2 Point4;
        private Vector2 Min;
        private Vector2 Max;
        private uint Count;
        private uint Padding0;
        private uint Padding1;
        private uint Padding2;

        public Curve(uint count = 2)
        {
            this = default(Curve);
            Min = new Vector2(0.0f, 0.0f);
            Max = new Vector2(1.0f, 1.0f);
            Count = count > MaxPoints ? MaxPoints : count;
            Point0 = new Vector2(0.0f, 0.0f);
            Point1 = new Vector2(1.0f, 1.0f);
        }
    }
}
