using System;

namespace Waldem
{
    internal enum ComponentKind
    {
        None = 0,
        Transform = 1,
        Camera = 2,
        RigidBody = 3,
        Light = 4
    }

    public class Entity
    {
        internal ulong EntityId;

        public ulong Id => EntityId;

        internal void __SetEntityId(ulong entityId)
        {
            EntityId = entityId;
        }

        public bool HasComponent<T>() where T : Component, new()
        {
            return InternalCalls.Entity_HasComponent(EntityId, (int)GetComponentKind(typeof(T)));
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
                return null;

            T component = new T();
            component.Attach(EntityId);
            return component;
        }

        public TransformComponent Transform   => GetComponent<TransformComponent>();
        public CameraComponent    Camera      => GetComponent<CameraComponent>();
        public RigidBodyComponent RigidBody   => GetComponent<RigidBodyComponent>();
        public LightComponent     Light       => GetComponent<LightComponent>();

        private static ComponentKind GetComponentKind(Type type)
        {
            if (type == typeof(TransformComponent))  return ComponentKind.Transform;
            if (type == typeof(CameraComponent))     return ComponentKind.Camera;
            if (type == typeof(RigidBodyComponent))  return ComponentKind.RigidBody;
            if (type == typeof(LightComponent))      return ComponentKind.Light;

            return ComponentKind.None;
        }
    }
}
