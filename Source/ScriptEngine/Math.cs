namespace Waldem
{
    public static class Math
    {
        public const float PI = (float)System.Math.PI;
        public const float Deg2Rad = PI / 180.0f;
        public const float Rad2Deg = 180.0f / PI;

        public static float Abs(float value) => System.Math.Abs(value);
        public static float Min(float a, float b) => System.Math.Min(a, b);
        public static float Max(float a, float b) => System.Math.Max(a, b);
        public static float Clamp(float value, float min, float max) => Max(min, Min(max, value));
        public static float Clamp01(float value) => Clamp(value, 0.0f, 1.0f);
        public static float Sign(float value) => value < 0.0f ? -1.0f : 1.0f;

        public static float Floor(float value) => (float)System.Math.Floor(value);
        public static float Ceil(float value) => (float)System.Math.Ceiling(value);
        public static float Round(float value) => (float)System.Math.Round(value);
        public static float Sqrt(float value) => (float)System.Math.Sqrt(value);
        public static float Pow(float value, float power) => (float)System.Math.Pow(value, power);

        public static float Sin(float radians) => (float)System.Math.Sin(radians);
        public static float Cos(float radians) => (float)System.Math.Cos(radians);
        public static float Tan(float radians) => (float)System.Math.Tan(radians);
        public static float Asin(float value) => (float)System.Math.Asin(value);
        public static float Acos(float value) => (float)System.Math.Acos(value);
        public static float Atan(float value) => (float)System.Math.Atan(value);
        public static float Atan2(float y, float x) => (float)System.Math.Atan2(y, x);

        public static float Lerp(float a, float b, float t) => a + (b - a) * Clamp01(t);
        public static float LerpUnclamped(float a, float b, float t) => a + (b - a) * t;
        public static Vector3 Lerp(Vector3 a, Vector3 b, float t) => a + (b - a) * Clamp01(t);
        public static Vector3 LerpUnclamped(Vector3 a, Vector3 b, float t) => a + (b - a) * t;

        public static float MoveTowards(float current, float target, float maxDelta)
        {
            float delta = target - current;
            if(Abs(delta) <= maxDelta)
                return target;

            return current + Sign(delta) * maxDelta;
        }

        public static Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDelta)
        {
            Vector3 delta = target - current;
            float distance = delta.magnitudeF;
            if(distance <= maxDelta || distance <= 0.0001f)
                return target;

            return current + delta * (maxDelta / distance);
        }

        public static float SmoothStep(float from, float to, float t)
        {
            t = Clamp01(t);
            t = t * t * (3.0f - 2.0f * t);
            return LerpUnclamped(from, to, t);
        }

        public static float Repeat(float t, float length)
        {
            if(length <= 0.0f)
                return 0.0f;

            return Clamp(t - Floor(t / length) * length, 0.0f, length);
        }

        public static float PingPong(float t, float length)
        {
            t = Repeat(t, length * 2.0f);
            return length - Abs(t - length);
        }

        public static float Distance(Vector3 a, Vector3 b) => (a - b).magnitudeF;
        public static float Dot(Vector3 a, Vector3 b) => a.x * b.x + a.y * b.y + a.z * b.z;
        public static Vector3 Cross(Vector3 a, Vector3 b)
        {
            return new Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
        }

        public static float Angle(Vector3 from, Vector3 to)
        {
            float denominator = from.magnitudeF * to.magnitudeF;
            if(denominator <= 0.0001f)
                return 0.0f;

            float dot = Clamp(Dot(from, to) / denominator, -1.0f, 1.0f);
            return Acos(dot) * Rad2Deg;
        }

        public static Vector3 Normalize(Vector3 value)
        {
            float magnitude = value.magnitudeF;
            if(magnitude <= 0.0001f)
                return Vector3.Zero;

            return value * (1.0f / magnitude);
        }

    }
}
