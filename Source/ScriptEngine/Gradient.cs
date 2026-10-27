using System.Runtime.InteropServices;

namespace Waldem
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Gradient
    {
        const uint MaxPoints = 5;

        private Vector4 Point0;
        private Vector4 Point1;
        private Vector4 Point2;
        private Vector4 Point3;
        private Vector4 Point4;
        private uint Count;
        private uint Padding0;
        private uint Padding1;
        private uint Padding2;

        public Gradient(uint count = 2)
        {
            this = default(Gradient);
            Count = count > MaxPoints ? MaxPoints : count;
            Point0 = new Vector4(1.0f, 1.0f, 1.0f, 0.0f);
            Point1 = new Vector4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}
