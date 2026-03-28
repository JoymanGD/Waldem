namespace Waldem
{
    public static class Input
    {
        public static bool IsKeyPressed(KeyCode keyCode)
        {
            return InternalCalls.Input_IsKeyPressed((int)keyCode);
        }

        public static bool IsMouseButtonPressed(MouseButton mouseButton)
        {
            return InternalCalls.Input_IsMouseButtonPressed((int)mouseButton);
        }
    }
}
