namespace Waldem
{
    public abstract class Debug
    {
        public static void Log(object message)
        {
            InternalCalls.LogInfo(message?.ToString() ?? "null");
        }

        public static void Warning(object message)
        {
            InternalCalls.LogWarning(message?.ToString() ?? "null");
        }

        public static void Error(object message)
        {
            InternalCalls.LogError(message?.ToString() ?? "null");
        }
    }
}
