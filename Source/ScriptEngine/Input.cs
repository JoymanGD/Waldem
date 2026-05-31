namespace Waldem
{
    public static class Input
    {
        public static bool GetKey(KeyCode keyCode)
        {
            return InternalCalls.Input_GetKey((int)keyCode);
        }

        public static bool GetKeyDown(KeyCode keyCode)
        {
            return InternalCalls.Input_GetKeyDown((int)keyCode);
        }

        public static bool GetMouse(MouseButton mouseButton)
        {
            return InternalCalls.Input_GetMouse((int)mouseButton);
        }

        public static bool GetMouseDown(MouseButton mouseButton)
        {
            return InternalCalls.Input_GetMouseDown((int)mouseButton);
        }

        public static float GetMouseDeltaX()
        {
            return InternalCalls.Input_GetMouseDeltaX();
        }

        public static float GetMouseDeltaY()
        {
            return InternalCalls.Input_GetMouseDeltaY();
        }
    }
}
