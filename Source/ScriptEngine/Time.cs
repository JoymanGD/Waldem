namespace Waldem
{
    public static class Time
    {
        public static float DeltaTime => InternalCalls.Time_GetDeltaTime();
        public static float FixedDeltaTime => InternalCalls.Time_GetFixedDeltaTime();
        public static float ElapsedTime => InternalCalls.Time_GetElapsedTime();
    }
}
