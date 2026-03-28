using System.Runtime.CompilerServices;

namespace Waldem
{
    internal static class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Entity_HasComponent(ulong entityId, int componentKind);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsKeyPressed(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Input_IsMouseButtonPressed(int mouseButton);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Time_GetDeltaTime();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Time_GetFixedDeltaTime();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float Time_GetElapsedTime();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetPosition(ulong entityId, out Vector3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetPosition(ulong entityId, ref Vector3 position);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetForward(ulong entityId, out Vector3 forward);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetUp(ulong entityId, out Vector3 up);
        
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetRight(ulong entityId, out Vector3 right);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_Translate(ulong entityId, ref Vector3 translation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetRotation(ulong entityId, out Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetRotation(ulong entityId, ref Vector3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_Rotate(ulong entityId, ref Vector3 rotationDelta);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_GetScale(ulong entityId, out Vector3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Transform_SetScale(ulong entityId, ref Vector3 scale);
    }
}
