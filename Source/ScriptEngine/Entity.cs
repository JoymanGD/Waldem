using System;

namespace Waldem
{
    internal enum ComponentKind
    {
        None = 0,
        Transform = 1,
        Camera = 2,
        RigidBody = 3
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

        public TransformComponent Transform => GetComponent<TransformComponent>();

        private static ComponentKind GetComponentKind(Type type)
        {
            if (type == typeof(TransformComponent))
                return ComponentKind.Transform;

            return ComponentKind.None;
        }
    }
}
